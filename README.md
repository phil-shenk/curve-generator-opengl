# curve-generator-opengl
an exercise in generating a 3D mesh representation of an arbitrary parametrized 3D curve, and rendering it in OpenGL

## Instructions:
```make curvegen```  
```curvegen <int path_segments> <int tube_segments> <float tube_radius>```
  
requires gcc, make, and GLEW, GLUT, opengl, libxmu, libxi, as well as my linear algebra library, linalglib (github.com/phil-shenk/linalglib)  
linalglib can be cloned to the same parent directory as curve-generator-opengl, or you can modify the makefile to point to your linalglib directory.  

The resulting binary can accept either 0 or 3 additional arguments.  
With no additional arguments supplied, the generated curve will have 200 path segments and 10 'tube segments' (a circumference of 2\*10=20 triangles), and a tube radius of 0.15 units.  
If you want to customize these parameters, usage of curvegen is as follows:  
```curvegen <int path_segments> <int tube_segments> <float tube_radius>```

For now, the parametrized curve that you wish to generate must be hard-coded as a function of the parameter ```0 < t < 1``` in the ```curve``` function of ```curvegen.py``` 

UPDATE:
To ensure that each ring of triangles matches perfectly with the previous one, the 'normal' vector to the path is chosen by projecting the previous normal vector onto the tangent plane at the current point on the path, as shown here:  
![orthogonal vector calculation math](https://github.com/phil-shenk/curve-generator-opengl/blob/main/screenshots/normal_projection_math.png)
  
## Screenshots 
Here are some screenshots of various curves:  
![screenshot 1](https://github.com/phil-shenk/curve-generator-opengl/blob/main/screenshots/refined_normal_calculation.png)  
![screenshot 1](https://github.com/phil-shenk/curve-generator-opengl/blob/main/screenshots/high_detail_screenshot1.png)  
![screenshot 1](https://github.com/phil-shenk/curve-generator-opengl/blob/main/screenshots/spiral_screenshot.png)  
![screenshot 1](https://github.com/phil-shenk/curve-generator-opengl/blob/main/screenshots/screenshot_1.png)  
