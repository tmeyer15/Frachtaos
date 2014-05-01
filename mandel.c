#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GLUT/glut.h>

#define pi 3.1415926
#define ORBIT_DEPTH 255

//#define VERBOSE

double xmin,ymin,xmax,ymax;
int w_mandel=400*2,h_mandel=300*2;
int w=400*2,h=300*2;

typedef struct point
{
	double x;
	double y;
} point;

typedef struct intpoint
{
	int x;
	int y;
} intpoint;

typedef struct rgb
{
	float r;
	float g;
	float b;
} rgb;

//Represents the bounds of the mandelbrot (so usually, (-2,-1)->(1,1))
point* startCoord;
point* endCoord;

//color scheme (plug in the number of iterations to get the color)
rgb* colorScheme;

//Final display color for each pixel of the fractal only (no lines drawn)
rgb* pixels;

//The point of the orbit that is to be drawn
intpoint* orbitDraw;

//a matrix of arrays that contain all of the points in each orbit - a lot of memory!
point*** orbits;

//Returns the sqaure magnitude of the vector/complex number
double squarMag(point* p)
{
	return p->x * p->x + p->y * p->y;
}

//Give it the old complex number and c, returns the next complex number using the mandelbrot mapping
point* mandelMap(point* oldPoint, point* c)
{
	point* newPoint = (point*) malloc(sizeof(point));
	double tmp = oldPoint->x * oldPoint->x - oldPoint->y * oldPoint->y + c->x;
	newPoint->y = 2 * oldPoint->x * oldPoint->y + c->y;
	newPoint->x = tmp;
	return newPoint;
}

//give it the width and height of the window size you want to fill
//give it an array that can fit all those pixels
//give it a widthxheight array of point** that it will fill with points from each pixel's orbit
void mandelbrot(int width, int height, rgb* pixelsToFill, point*** allOrbits)
{
	int j;
	#pragma omp parallel for
	for (j = 0; j < height; j++)
	{
		int i;
		for (i = 0; i < width; i++)
		{
			int fillPixel = j * width + i;
			point* originPoint = (point*) malloc(sizeof(point));
			originPoint->x = i * (endCoord->x - startCoord->x) / width + startCoord->x;
			originPoint->y = j * (endCoord->y - startCoord->y) / height + startCoord->y;
			
			allOrbits[fillPixel] = (point**) malloc(sizeof(point*) * ORBIT_DEPTH);
			allOrbits[fillPixel][0] = originPoint;
			
			point* newPoint = (point*) malloc(sizeof(point));
			newPoint->x = originPoint->x;
			newPoint->y = originPoint->y;
			
			int iterCount = 1;
			
			newPoint = mandelMap(newPoint, originPoint);
			allOrbits[fillPixel][iterCount] = newPoint;
			
			while (iterCount < ORBIT_DEPTH && squarMag(newPoint) < 4.0)
			{
				newPoint = mandelMap(newPoint, originPoint);
				iterCount++;
				allOrbits[fillPixel][iterCount] = newPoint;
			}
			
			if (iterCount != ORBIT_DEPTH)
			{
				allOrbits[fillPixel][iterCount+1] = NULL;
			}
			
			pixelsToFill[fillPixel] = colorScheme[iterCount];
			//pixelsToFill[fillPixel].r = iterCount / 255.;
		}
	}
}

void display(void)
{
	#ifdef VERBOSE
	printf("Redraw\n");
	#endif
	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glDrawPixels(w_mandel, h_mandel, GL_RGB, GL_FLOAT, pixels);
	
	if (orbits != NULL)
	{
		#ifdef VERBOSE
		printf("begin draw orbit\n");
		#endif
		glBegin(GL_LINES);
		glColor3f(1.0,0.0,0.0);
		glLineWidth (1.0);
		int orbitToDraw = (h_mandel - orbitDraw->y - 1) * w_mandel + orbitDraw->x;
		#ifdef VERBOSE
		printf("(x,y)=(%d,%d)\n", (int)orbitDraw->x, (int)orbitDraw->y);
		#endif
		int x;
		for (x = 1; x < ORBIT_DEPTH; x++)
		{
			if (orbits[orbitToDraw][x] == NULL)
			{
				break;
			}
			#ifdef VERBOSE
			printf("(x,y)=(%9.8f,%9.8f)\n",orbits[orbitToDraw][x]->x,orbits[orbitToDraw][x]->y);
			#endif	
			
			double xscale = endCoord->x - startCoord->x;
			double yscale = endCoord->y - startCoord->y;
			double xaverage = (endCoord->x + startCoord->x) / 2.0;
			double yaverage = (endCoord->y + startCoord->y) / 2.0;
			glVertex2f((orbits[orbitToDraw][x-1]->x - startCoord->x) * 2.0 / xscale - 1.0, (orbits[orbitToDraw][x-1]->y - startCoord->y) * 2.0 / yscale - 1.0);
			glVertex2f((orbits[orbitToDraw][x]->x - startCoord->x) * 2.0 / xscale - 1.0, (orbits[orbitToDraw][x]->y - startCoord->y) * 2.0 / yscale - 1.0);
		}
		glEnd();
		
		#ifdef VERBOSE
		printf("end draw orbit\n");
		#endif
	}
	glutSwapBuffers();
}

void mouse(int button,int state,int xscr,int yscr)
{
	if(button==GLUT_LEFT_BUTTON)
	{
		if(state==GLUT_DOWN)
		{
			orbitDraw->x = xscr;
			orbitDraw->y = yscr;
			#ifdef VERBOSE
			printf("(x,y)=(%d,%d)\n",xscr,yscr);
			#endif
			glutPostRedisplay(); // callback
		}
	}
}

void motion(int xscr,int yscr)
{
	
}
void keyfunc(unsigned char key,int xscr,int yscr)
{
	if(key=='q')
	{
		exit(0);
	}
}

void reshape(int wscr,int hscr)
{
		
	//We only want to update if the window is changed by a large enough amount
	if (abs(wscr - w_mandel) > 50 || abs(hscr - h_mandel) > 50)
	{
		w=wscr; h=hscr;
			
		//Not really sure what this does. I'm leaving it in because it was in
		//a previous project from a long time ago in a classroom far far away
		glViewport(0,0,(GLsizei)w,(GLsizei)h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		xmin=ymin=0.0; xmax=ymax=1.0;
		if(w<=h)
		{
			ymax=1.0*(GLdouble)h/(GLdouble)w;
		}
		else
		{
			xmax=1.0*(GLdouble)w/(GLdouble)h;
		}
		gluOrtho2D(xmin,xmax,ymin,ymax);
		glMatrixMode(GL_MODELVIEW);

		point*** createOrbits = (point***) malloc(sizeof(point**) * h * w);
		rgb* newPixels = (rgb*) malloc(sizeof(rgb) * h * w);
		mandelbrot(w, h, newPixels, createOrbits);
		orbits = createOrbits;
		
		//Now that we've done the computation, set the new mandelbrot pixels
		rgb* oldPixels = pixels;
		pixels = newPixels;
		w_mandel = w;
		h_mandel = h;
		free (oldPixels);
	}
}

int main(int argc, char** argv)
{
	//OpenGL boilerplate stuff
	glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); // single buffering
    glutInitWindowSize(w,h);                     // for double use GLUT_DOUBLE
    glutInitWindowPosition(100,50);
    glutCreateWindow("Mandelbrot Set (press 'q' to quit)");
	
    glClearColor(1.0,1.0,1.0,0.0);
    glShadeModel(GL_SMOOTH);
	
    glutDisplayFunc(display);		// register callback functions
    glutIdleFunc(NULL);				// no animation
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyfunc);
    glutReshapeFunc(reshape);
	
	//End OpenGL Boilerplate stuff
	
	//Color scheme from http://skr1986.wordpress.com/2012/11/12/mandelbrot-set-implementation-using-c-and-opengl/
	colorScheme = (rgb*) malloc(sizeof(rgb) * 1000);
	double r, g, b;
	int x = 0;
	for(r = 0.1; r <= 0.9; r= r+0.1)
	{
		for(g = 0.1; g <= 0.9; g = g+0.1)
		{
			for(b = 0.1; b <= 0.9; b = b+0.1)
			{
				colorScheme[x].r = r;
				colorScheme[x].g = g;
				colorScheme[x].b = b;
				x++;
			}
		}
	}

	orbitDraw = (intpoint*) malloc(sizeof(intpoint));
	orbitDraw->x = 0;
	orbitDraw->y = 0;
	
	startCoord = (point*) malloc(sizeof(point));
	startCoord->x = -3.0;
	startCoord->y = -2.0;
	
	endCoord = (point*) malloc(sizeof(point));
	endCoord->x = 2.0;
	endCoord->y = 1.5;
	
	orbits = NULL;
	
	point*** createOrbits = (point***) malloc(sizeof(point**) * h_mandel * w_mandel);
	pixels = (rgb*) malloc(sizeof(rgb) * w_mandel * h_mandel);
	mandelbrot(w, h, pixels, createOrbits);
	orbits = createOrbits;
	
	glutMainLoop();					// here we go!
	
	//point* points = (point*) malloc(sizeof(point) * N);
	
}


