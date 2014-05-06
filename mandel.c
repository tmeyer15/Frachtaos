#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <GLUT/glut.h>

#define pi 3.1415926
#define ORBIT_DEPTH 16000
#define DRAW_DEPTH 999

//#define VERBOSE

#define START_X_RESOLUTION 400*2
#define START_Y_RESOLUTION 300*2

char stringOfNumberOfIterations[50];
double xmin,ymin,xmax,ymax;
int w_mandel=START_X_RESOLUTION,h_mandel=START_Y_RESOLUTION;
int w=START_X_RESOLUTION,h=START_Y_RESOLUTION;

//void (*mappingFunction)(point*, point*) = juliaMap;

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


point* orbitMappingToDraw;
int numPointsToDraw;

void drawString(char* s)
{
	int k;
	for(k=0;k<strlen(s);k++)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, s[k]);
}

//Returns the sqaure magnitude of the vector/complex number
double squarMag(point* p)
{
	return p->x * p->x + p->y * p->y;
}

//Give it the old complex number and c, returns the next complex number using the mandelbrot mapping
void mandelMap(point* oldPoint, point* c)
{
	double tmp = oldPoint->x * oldPoint->x - oldPoint->y * oldPoint->y + c->x;
	oldPoint->y = 2 * oldPoint->x * oldPoint->y + c->y;
	oldPoint->x = tmp;
}

#define JULIA_CONST_ONE -0.8
#define JULIA_CONST_TWO 0.156
void juliaMap(point* oldPoint, point* nothing)
{
	double tmp = oldPoint->x * oldPoint->x - oldPoint->y * oldPoint->y + JULIA_CONST_ONE;
	oldPoint->y = 2 * oldPoint->x * oldPoint->y + JULIA_CONST_TWO;
	oldPoint->x = tmp;
}

/* Iterates the mapping until it escapes or reaches ORBIT_DEPTH
   Send an array of points of size ORBIT_DEPTH to get a null terminated
   list of all of the points in the mapping, or send null to ignore
 */
rgb iterateMapping(int i, int j, int width, int height, point* pointsInOrbit, int* numPointsInOrbit, void (*mapFunction)(point*, point*), int depthToSearch)
{
	point originPoint;
	originPoint.x = i * (endCoord->x - startCoord->x) / width + startCoord->x;
	originPoint.y = j * (endCoord->y - startCoord->y) / height + startCoord->y;
	
	if (pointsInOrbit != NULL)
	{
		*numPointsInOrbit = 0;
		pointsInOrbit[0].x = originPoint.x;
		pointsInOrbit[0].y = originPoint.y;
		(*numPointsInOrbit)++;
	}
	
	point newPoint;
	newPoint.x = originPoint.x;
	newPoint.y = originPoint.y;
	
	int iterCount = 1;
	
	mandelMap(&newPoint, &originPoint);
	
	while (iterCount < depthToSearch && squarMag(&newPoint) < 4.0)
	{
		if (pointsInOrbit != NULL)
		{
			pointsInOrbit[iterCount].x = newPoint.x;
			pointsInOrbit[iterCount].y = newPoint.y;
			(*numPointsInOrbit)++;
		}
		
		mapFunction(&newPoint, &originPoint);
		iterCount++;
	}
	
	return colorScheme[iterCount];
}

//give it the width and height of the window size you want to fill
//give it an array that can fit all those pixels
//give it a widthxheight array of point** that it will fill with points from each pixel's orbit
void paintMapping(int width, int height, rgb* pixelsToFill, void (*mapFunction)(point*, point*))
{
	int j;
	#pragma omp parallel for
	for (j = 0; j < height; j++)
	{
		int i;
		for (i = 0; i < width; i++)
		{
			int fillPixel = j * width + i;
			pixelsToFill[fillPixel] = iterateMapping(i, j, width, height, NULL, NULL, mapFunction, DRAW_DEPTH);
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
	glRasterPos2f(-1.0,-1.0);
	glDrawPixels(w_mandel, h_mandel, GL_RGB, GL_FLOAT, pixels);
	
	if (orbitMappingToDraw != NULL)
	{
		int periodOfOrbit = 0;
		//Find if the orbit is chaotic or not
		int x;
		for (x = numPointsToDraw - 1; x >= 0; x--)
		{
			int y;
			for (y = x - 1; y >= 0 ; y--)
			{
				if (orbitMappingToDraw[x].x == orbitMappingToDraw[y].x && orbitMappingToDraw[x].y == orbitMappingToDraw[y].y)
				{
					periodOfOrbit = (int) abs(x - y);
					break;
				}
			}
			if (periodOfOrbit)
			{
				break;
			}
		}
		printf("%d\n", periodOfOrbit);
		
		glColor3f(1.0,1.0,1.0);
		
		sprintf(stringOfNumberOfIterations, "Periodicity: %d", periodOfOrbit);
		glRasterPos2f(-.99, .90);//position of the text
		drawString(stringOfNumberOfIterations);
		
		char* chaoticOrNot;
		if (periodOfOrbit)
		{
			chaoticOrNot = "Not Chaotic";
		}
		else
		{
			chaoticOrNot = "Chaotic";
		}
		glRasterPos2f(-.99, .80);//position of the text
		drawString(chaoticOrNot);
		
		#ifdef VERBOSE
		printf("begin draw orbit\n");
		#endif
		glColor3f(1.0,0.0,0.0);
		glBegin(GL_LINES);
		glLineWidth (1.0);
		//Draw all of the points in the orbit
		
		for (x = 1; x < numPointsToDraw; x++)
		{
			#ifdef VERBOSE
			printf("(x,y)=(%9.8f,%9.8f)\n",orbitMappingToDraw[x].x,orbitMappingToDraw[x].y);
			#endif	
			
			double xscale = endCoord->x - startCoord->x;
			double yscale = endCoord->y - startCoord->y;
			glVertex2f((orbitMappingToDraw[x-1].x - startCoord->x) * 2.0 / xscale - 1.0, (orbitMappingToDraw[x-1].y - startCoord->y) * 2.0 / yscale - 1.0);
			glVertex2f((orbitMappingToDraw[x].x - startCoord->x) * 2.0 / xscale - 1.0, (orbitMappingToDraw[x].y - startCoord->y) * 2.0 / yscale - 1.0); 
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
			#ifdef VERBOSE
			printf("(x,y)=(%d,%d)\n",xscr,yscr);
			#endif
			
			iterateMapping(xscr, h-yscr, w, h, orbitMappingToDraw, &numPointsToDraw, mandelMap, ORBIT_DEPTH);
			
			#ifdef VERBOSE
			printf("Iterations of orbit: (%d)\n", numPointsToDraw);
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
	//if (abs(wscr - w_mandel) > 50 || abs(hscr - h_mandel) > 50)
	//{
		w=wscr; h=hscr;
			
		//Not really sure what this does. I'm leaving it in because it was in
		//a previous project from a long time ago in a classroom far far away
	/*
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
		glMatrixMode(GL_MODELVIEW);*/

	
		rgb* newPixels = (rgb*) malloc(sizeof(rgb) * h * w);
		paintMapping(w, h, newPixels, mandelMap);
		
		//Now that we've done the computation, set the new mandelbrot pixels
		rgb* oldPixels = pixels;
		pixels = newPixels;
		w_mandel = w;
		h_mandel = h;
		free (oldPixels);
	//}
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
	
	startCoord = (point*) malloc(sizeof(point));
	startCoord->x = -3.0;
	startCoord->y = -2.0;
	
	endCoord = (point*) malloc(sizeof(point));
	endCoord->x = 2.0;
	endCoord->y = 2.0;
	
	orbitMappingToDraw = (point*) malloc(sizeof(point) * ORBIT_DEPTH);
	pixels = (rgb*) malloc(sizeof(rgb) * w_mandel * h_mandel);
	paintMapping(w, h, pixels, mandelMap);
	
	glutMainLoop();					// here we go!
	
	//point* points = (point*) malloc(sizeof(point) * N);
	
}


