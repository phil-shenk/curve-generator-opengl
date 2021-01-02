CC       = gcc 
CFLAGS   = -O3 -Wall 
LIBS      = -lXi -lXmu -lglut -lGLEW -lGLU -lm -lGL
OBJDIR   = ../linalglib
OBJS     = $(OBJDIR)/linalglib.o initShader.o

curvegen: curvegen.c $(OBJS)
	$(CC) -o curvegen curvegen.c $(OBJS) $(CFLAGS) $(LIBS)

$(OBJDIR)/%.o: %.c %.h
	$(CC) -c @< -o $@ $(CFLAGS)

