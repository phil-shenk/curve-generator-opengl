/*
 * template.c
 *
 * An OpenGL source code template.
 */


#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#include <stdio.h>

#include <math.h>

#include "initShader.h"
#include "../linalglib/linalglib.h"

#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

int num_vertices; //allocate later, need access to scope in multiple functions
GLuint ctm_location;
mat4 ctm = {
	{1,0,0,0},
	{0,1,0,0},
	{0,0,1,0},
	{0,0,0,1}
};
float theta = 0.0;
int window_size = 1024;

float bx_prev;
float by_prev;
float bz_prev;

float inertialAngularSpeed = 0;
vec4 inertialAxis = {1,0,0,0};
int coastingEnabled = 0;
int coasting = 0;

int outside_ball = 1;

/* 
 * vector function representing the curve as a function of parameter 't'
 * t = value that parametrizes the curve from 0 to 1
 */
vec4 curve(float t)
{
	vec4 result = {
		0.9*cos(2*M_PI*t),
		0.9*sin(2*M_PI*t),
		0.3*sin(2*M_PI*4.0*t),
		0.0
	};
	/* interesting twist
	vec4 result = {
		cos(2.9*M_PI*t),
		sin(2*2.50*M_PI*t),
		sin(2*2*M_PI*t),
		0.0
	};
	*/
	/* funky fresh
	vec4 result = {
		cos(2.9*M_PI*t),
		sin(M_PI*t*0.5),
		fmod(tan(M_PI*5.0*t), 1.0),
		0.0
	};
	*/
	/* basic spiral curve
	vec4 result = {
		cos(2*M_PI*1*t),
		sin(2*M_PI*1*t),
		2*t - 1,
		0
	};
	*/
	return result;
}
/*
 * vector function to get the curve using the index of the curve segment and the number of total curve segments
 */
vec4 curve_from_index(int i, int numSegments){
	return curve(((float)i)/((float)numSegments));
}

/*
 * vector function representing the tangent of the curve as a function of parameter 't'
 */
vec4 tangent(int i, int numSegments){
	// tangent is approximated as the difference between the vector at this point and the vector at the *next* point
	vec4 thisPoint = curve_from_index(i,   numSegments);
	vec4 nextPoint = curve_from_index(i+1, numSegments);
	vec4 tang = normalize_v4(subtract_v4(nextPoint, thisPoint));

	return tang;
}

/* 
 * curvature = second derivative of curve w.r.t. the parameter
 * i.e. derivative of tangent
 */
vec4 curvature(int i, int numSegments){
	vec4 thisTangent = tangent(i,   numSegments);
	vec4 nextTangent = tangent(i+1, numSegments);
	vec4 curv = normalize_v4(subtract_v4(nextTangent, thisTangent));
	return curv;
}

/*
 * vector function representing a normal of the curve as a function of parameter 't'
 */
vec4 normal(int i, int numSegments, vec4 prev_normal){
	vec4 tang = tangent(i, numSegments);
	vec4 tang_projection = project_v4(prev_normal, tang);
	vec4 norml = subtract_v4(prev_normal, tang_projection);
	norml = normalize_v4(norml);
	//could also use project_onto_plane() function that I made for this purpose.. and make sure to normalize
	return norml;
}
// DEPRECATED: no longer need to use curvature()
vec4 old_normal(int i, int numSegments){
	vec4 curv = curvature(i, numSegments);
	vec4 tang = tangent(i, numSegments);
	vec4 norml = cross_v4(tang, curv);
	if(magnitude_v4(norml) < 0.0){
		norml = scale_v4(-1.0, norml);
	}
	return norml;
}
vec4 tubePoint(int i, int pathsegs, int j, int tubesegs, float twists, float tubeRadius, vec4 prev_normal){
	vec4 curvepos = curve_from_index(i, pathsegs);
	vec4 vnorm = normal(i, pathsegs, prev_normal);
	vec4 P0 = scale_v4(tubeRadius, vnorm);
	P0.w = 1; // just to make a point HA

	vec4 vtang = tangent(i, pathsegs);
	mat4 rot = arb_rotation_origin(2*M_PI*j/tubesegs, vtang.x, vtang.y, vtang.z);
	vec4 tubept = multiply_m4v4(rot, P0);

	//shift to its proper lotaion by adding curvepos
	tubept = add_v4(tubept, curvepos);
	return tubept;
}
	
void populateSpringVertices(int pathsegs, int tubesegs, float twists, float tubeRadius, vec4 vertices[]){
	// must have an initial direction for the first normal
	vec4 prev_normal = {-0.5, 0.1, 0.3, 0.0};// #TODO: check to make sure that the curve doesn't start out in this direction..
	// make first end cap
	for(int j=0; j<tubesegs; j++){
		// 1st end cap
		vec4 v1 = tubePoint(0,  pathsegs,j,  tubesegs,twists,tubeRadius,prev_normal);
		vec4 v3 = tubePoint(0,  pathsegs,j+1,tubesegs,twists,tubeRadius,prev_normal);
		vec4 v2 = curve_from_index(0, pathsegs);
		v2.w = 1;
		// 1st endcap triangle
		vertices[(tubesegs*pathsegs + j)*6 + 0]   = v1;
		vertices[(tubesegs*pathsegs + j)*6 + 1]   = v2;
		vertices[(tubesegs*pathsegs + j)*6 + 2]   = v3;
	}
	// make tube segment
	for(int i=0; i<pathsegs; i++){
		for(int j=0; j<tubesegs; j++){
			vec4 v1 = tubePoint(i,  pathsegs,j,  tubesegs,twists,tubeRadius, prev_normal);
			vec4 v2 = tubePoint(i+1,pathsegs,j,  tubesegs,twists,tubeRadius, prev_normal);
			vec4 v3 = tubePoint(i,  pathsegs,j+1,tubesegs,twists,tubeRadius, prev_normal);
			vec4 v4 = tubePoint(i+1,pathsegs,j+1,tubesegs,twists,tubeRadius, prev_normal);
			// triangle A
			vertices[(tubesegs*i + j)*6 + 0]   = v3;
			vertices[(tubesegs*i + j)*6 + 1]   = v2;
			vertices[(tubesegs*i + j)*6 + 2]   = v1;
			//triangle B	
			vertices[(tubesegs*i + j)*6 + 3]   = v3;
			vertices[(tubesegs*i + j)*6 + 4]   = v4;
			vertices[(tubesegs*i + j)*6 + 5]   = v2;
		}
		// update prev_normal with the new current normal
		prev_normal = normal(i, pathsegs, prev_normal);
	}
	// make last end cap
	for(int j=0; j<tubesegs; j++){
		// last end cap
		vec4 v4 = tubePoint(pathsegs,  pathsegs,j,  tubesegs,twists,tubeRadius,prev_normal);
		vec4 v6 = tubePoint(pathsegs,  pathsegs,j+1,tubesegs,twists,tubeRadius,prev_normal);
		vec4 v5 = curve_from_index(pathsegs, pathsegs);
		v5.w = 1;
		// last endcap triangle
		vertices[(tubesegs*pathsegs + j)*6 + 3]   = v4;
		vertices[(tubesegs*pathsegs + j)*6 + 4]   = v6;
		vertices[(tubesegs*pathsegs + j)*6 + 5]   = v5;
	}
	return;
}

void populateSpringColors(int n, vec4 colors[], int tubesegs){
	for(int i=0; i<n; i++){
		float r1 = (float)rand()/(float)(RAND_MAX);
		float g1 = (float)rand()/(float)(RAND_MAX);
		float b1 = (float)rand()/(float)(RAND_MAX);
		float r2 = (float)rand()/(float)(RAND_MAX);
		float g2 = (float)rand()/(float)(RAND_MAX);
		float b2 = (float)rand()/(float)(RAND_MAX);
		vec4 c1 = {r1, g1, b1, 1};
		vec4 c2 = {r2, g2, b2, 1};
		if(i%tubesegs==0){
			c1.x = 0.8;
			c1.y = 0.0;
			c1.z = 0.0;
			c2 = c1;
		}
		colors[i*6]   = c1;
		colors[i*6+1] = c1;
		colors[i*6+2] = c1;//by value, so dont need to copy :) 
		colors[i*6+3] = c2;
		colors[i*6+4] = c2;
		colors[i*6+5] = c2;//by value, so dont need to copy :) 
	}
	return;
}


void init(int pathsegs, int tubesegs, float twists, float tubeRadius)
{
    // add another tubesegs to allow endcaps
    int num_triangle_pairs = tubesegs*pathsegs + tubesegs;
    printf("%i",num_triangle_pairs);
    num_vertices = num_triangle_pairs*6;
    vec4 vertices[num_vertices];
    vec4 colors[num_vertices];
    populateSpringVertices(pathsegs, tubesegs, twists, tubeRadius, vertices);
    populateSpringColors(num_triangle_pairs, colors, tubesegs);

    GLuint program = initShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);

    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *) sizeof(vertices));

    ctm_location = glGetUniformLocation(program, "ctm");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDepthRange(1,0);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_LINE);

    glUniformMatrix4fv(ctm_location, 1, GL_FALSE, (GLfloat *) &ctm);

    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    glutSwapBuffers();
}

void keyboard(unsigned char key, int mousex, int mousey)
{
	if(key == 'q')
	{
		glutLeaveMainLoop();
	}
	if(key == 'c')
	{
		if(coastingEnabled == 0)
		{
			coastingEnabled = 1;
		}else{
			coastingEnabled = 0;
			coasting = 0;
		}
	}

    //glutPostRedisplay();
    if(key == 'p')
    {
	    ctm = multiply_m4(ctm,scaling(1.02,1.02,1.02));
    }
    if(key == 'o')
    {
	    ctm = multiply_m4(ctm, scaling(1/1.02,1/1.02,1/1.02));
    }
}

void mouse(int button, int state, int x, int y)
{
	if(button == 3)
	{
		ctm = multiply_m4(ctm, scaling(1.02,1.02,1.02));
		glutPostRedisplay();
	}
	if(button == 4)
	{
		ctm = multiply_m4(ctm, scaling(1/1.02,1/1.02,1/1.02));
		glutPostRedisplay();
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if(coastingEnabled==1)
		{
			coasting = 0;
		}
		//set previous vector to clicked ball position
		x -= (window_size/2);
		y -= (window_size/2);//256;
		y *= -1;
	
		bx_prev = (float)x/((float)window_size/2.0);
		by_prev = (float)y/((float)window_size/2.0);
		//bz_prev = 0;
		if(pow(bx_prev,2) + pow(by_prev, 2) <= 1)
		{
			outside_ball = 1;
			bz_prev = sqrt(1 - pow(bx_prev,2) - pow(by_prev,2));
		}
	}
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		if (coastingEnabled == 1)
		{
			coasting = 1;
		}else{
			coasting = 0;
		}
	}
}

void motion(int x, int y)
{
	x -= (window_size/2);
	y -= (window_size/2);
	y *= -1;

	float bx = (float)x/((float)window_size/2.0);
	float by = (float)y/((float)window_size/2.0);
	// ignore clicks outside the r=1 ball
	if(pow(bx,2)+pow(by,2) > 1)
	{
		outside_ball = 1;
		return;
	}
	else{
		float bz = sqrt(1 - pow(bx,2) - pow(by,2));

		// make sure not the ball doesn't jump around when you drag away from the sphere and back on
		if(outside_ball == 0)
		{
			// the previous x,y,z SHOULD ALWAYS be initialized by now by the mouseDown function
			vec4 new = {bx,by,bz,0};
			vec4 old = {bx_prev, by_prev, bz_prev, 0};
			// motion = new-old
			// vec4 movement = subtract_v4(new,old);
			float newDotOld = dot_v4(new,old);
			float magNew = magnitude_v4(new);
			float magOld = magnitude_v4(old);
			float theta = acos(newDotOld/(magNew*magOld));
			// more like dtheta/dframe
			// ideally would know how often idle() is called to get the speed right
			// for now just estimating 1/5 which seems visually fine, not too fast
			inertialAngularSpeed = theta/3;
	
			vec4 rotAx = cross_v4(old, new);
			inertialAxis = rotAx;
				
			ctm = multiply_m4(arb_rotation_origin(theta, rotAx.x, rotAx.y, rotAx.z), ctm);
		}
		outside_ball = 0;
	
		// maintain the 'previous' values of ball position
		bx_prev = bx;
		by_prev = by;
		bz_prev = bz;
	}
}


void reshape(int width, int height)
{
    glViewport(0, 0, window_size, window_size);
}

void idle(void){
	if(coasting == 1)
	{
		ctm = multiply_m4(arb_rotation_origin(inertialAngularSpeed, inertialAxis.x, inertialAxis.y, inertialAxis.z), ctm);
	}
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
    int pathsegs=200;
    int tubesegs=10;
    float twists=3.0;
    float tubeRadius=0.15;

    if(argc==5){
	printf("using arguments\n");
        pathsegs = atoi(argv[1]);
        tubesegs = atoi(argv[2]);
        twists   = atof(argv[3]);
        tubeRadius = atof(argv[4]);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_size, window_size);
    glutInitWindowPosition(100,100);
    glutCreateWindow("spiral season");
    glewInit();
    init(pathsegs, tubesegs, twists, tubeRadius);
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    // mouse motion listener
    glutMotionFunc(motion);
    glutReshapeFunc(reshape);
    glutMainLoop();

    return 0;
}
