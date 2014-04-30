#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GLUT/glut.h>

#define pi 3.1415926
#define ORBIT_DEPTH 255

double xmin,ymin,xmax,ymax;
int w_mandel=400,h_mandel=300;
int w=400,h=300;


typedef struct point
{
	double x;
	double y;
} point;

typedef struct rgb
{
	float r;
	float g;
	float b;
} rgb;

rgb* colorScheme;
rgb* pixels;
point* orbitDraw;
point* center;
point*** orbits;

//Returns the sqaure magnitude of the vector/complex number
double squarMag(point* p)
{
	return p->x * p->x + p->y * p->y;
}

point* mandelMap(point* oldPoint, point* c)
{
	point* newPoint = (point*) malloc(sizeof(point));
	double tmp = oldPoint->x * oldPoint->x - oldPoint->y * oldPoint->y + c->x;
	newPoint->y = 2 * oldPoint->x * oldPoint->y + c->y;
	newPoint->x = tmp;
	return newPoint;
}

void mandelbrot(int width, int height, rgb* pixelsToFill)
{
	int j;
	point*** createOrbits = (point***) malloc(sizeof(point**) * height * width);
	#pragma omp parallel for
	for (j = 0; j < height; j++)
	{
		int i;
		for (i = 0; i < width; i++)
		{
			int fillPixel = j * width + i;
			createOrbits[fillPixel] = (point**) malloc(sizeof(point*) * ORBIT_DEPTH);
			
			point* originPoint = (point*) malloc(sizeof(point));
			originPoint->x = -2.0 + 4.0 * i / width;
			originPoint->y = -1.5 + 3.0 * j / height;
			
			createOrbits[fillPixel][0] = originPoint;
			
			point* newPoint = (point*) malloc(sizeof(point));
			newPoint->x = -2.0 + 4.0 * i / width;
			newPoint->y = -1.5 + 3.0 * j / height;
			
			int iterCount = 1;
			
			newPoint = mandelMap(newPoint, originPoint);
			createOrbits[fillPixel][iterCount] = newPoint;
			
			while (iterCount < ORBIT_DEPTH && squarMag(newPoint) < 4.0)
			{
				newPoint = mandelMap(newPoint, originPoint);
				iterCount++;
				createOrbits[fillPixel][iterCount] = newPoint;
			}
			
			if (iterCount != ORBIT_DEPTH)
			{
				createOrbits[fillPixel][iterCount+1] = NULL;
			}
			
			pixelsToFill[fillPixel] = colorScheme[iterCount];
			//pixelsToFill[fillPixel].r = iterCount / 255.;
		}
	}
	orbits = createOrbits;
}

void display(void)
{
	printf("Redraw\n");
	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glDrawPixels(w_mandel, h_mandel, GL_RGB, GL_FLOAT, pixels);
	
	if (orbits != NULL)
	{
		printf("begin draw orbit\n");
		glBegin(GL_LINES);
		glColor3f(1.0,0.0,0.0);
		glLineWidth (1.0);
		int orbitToDraw = (h_mandel - orbitDraw->y - 1) * w_mandel + orbitDraw->x;
		printf("(x,y)=(%d,%d)\n", (int)orbitDraw->x, (int)orbitDraw->y);
		int x;
		for (x = 1; x < ORBIT_DEPTH; x++)
		{
			if (orbits[orbitToDraw][x] == NULL)
			{
				break;
			}
			printf("(x,y)=(%9.8f,%9.8f)\n",orbits[orbitToDraw][x]->x,orbits[orbitToDraw][x]->y);
			glVertex2f(orbits[orbitToDraw][x-1]->x / 2.0, orbits[orbitToDraw][x-1]->y * 2.0 / 3.0);
			glVertex2f(orbits[orbitToDraw][x]->x / 2.0  , orbits[orbitToDraw][x]->y * 2.0 / 3.0);
		}
		glEnd();
		printf("end draw orbit\n");
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
			printf("(x,y)=(%d,%d)\n",xscr,yscr);
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

		rgb* newPixels = (rgb*) malloc(sizeof(rgb) * h * w);
		mandelbrot(w, h, newPixels);
		
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

	orbitDraw = (point*) malloc(sizeof(point));
	orbitDraw->x = 0.0;
	orbitDraw->y = 0.0;
	
	orbits = NULL;
	
	center = (point*) malloc(sizeof(point));
	center->x = 0.0;
	center->y = 0.0;
	
	pixels = (rgb*) malloc(sizeof(rgb) * w_mandel * h_mandel);
	mandelbrot(w, h, pixels);
	
	glutMainLoop();					// here we go!
	
	//point* points = (point*) malloc(sizeof(point) * N);
	
}


