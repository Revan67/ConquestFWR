//---------------------------------------------------------------------------
// pci GL Text functions
//---------------------------------------------------------------------------

#include "display.h"
extern IDisplay *GL;

#define FONT 0x0

float Text_w = 0;
float Text_h = 0;

#define PT(x,y) glVertex2f(w*x/100,-h*y/100)

#define Hbl PT(0,0);PT(0,50);
#define Htl PT(0,50);PT(0,99);
#define Hbr PT(99,0);PT(99,50);
#define Htr PT(99,50);PT(99,99);

#define MM  PT(0,50);PT(99,50);

#define Otl PT(0,50);PT(50,99);
#define Otr PT(50,99);PT(99,50);
#define Obl PT(0,50);PT(50,0);
#define Obr PT(50,0);PT(99,50);

#define LL  PT(0,0);PT(0,99);
#define RR  PT(99,0);PT(99,99);

#define Bt  PT(0,99);PT(99,75);
#define Bi  PT(25,50);PT(99,75);
#define Bo  PT(25,50);PT(99,25);
#define Bb  PT(0,0);PT(99,25);

#define Ct  PT(0,50);PT(99,99);
#define Cb  PT(0,50);PT(99,0);

#define Dt  PT(0,99);PT(99,50);
#define Db  PT(0,0);PT(99,50);

#define TT  PT(0,99);PT(99,99);
#define BB  PT(0,0);PT(99,0);

#define Em  PT(0,50);PT(50,50);

#define Gm  PT(99,50);PT(50,50);

#define II  PT(50,0);PT(50,99);

#define Jb  PT(50,0);PT(0,25);

#define Ku  PT(0,50);PT(99,99);
#define Kd  PT(0,50);PT(99,0);

#define Md  PT(0,99);PT(50,25);
#define Mu  PT(50,25);PT(99,99);

#define Qt  PT(50,50);PT(99,0);

#define St  PT(50,99);PT(99,99);
#define Sb  PT(0,0);PT(50,0);

#define Vl  PT(50,0);PT(0,99);
#define Vr  PT(50,0);PT(99,99);

#define Wu  PT(0,0);PT(50,50);
#define Wd  PT(50,50);PT(99,0);

#define Xl  PT(0,99);PT(99,0);
#define Xr  PT(0,0);PT(99,99);

#define Yl  PT(50,50);PT(0,99);
#define Yr  PT(50,50);PT(99,99);
#define Yb  PT(50,0);PT(50,50);

void glutTextInit (float w, float h)
{
	Text_w = w;
	Text_h = h;

	#define OPEN(i) glNewList(FONT+i,GL_COMPILE);glBegin(GL_LINES);
	#define CLOSE()	glEnd();glEndList();

	for (int i=0; i<=128; i++)
	{
		switch (i)
		{
			case ' ':
				OPEN(i); CLOSE();
				break;

			case 'a':
				OPEN(i); Hbl; Otl; Otr; Hbr; MM; CLOSE();
				break;

			case 'b':
				OPEN(i); LL; Bt; Bi; Bo; Bb; CLOSE();
				break;

			case 'c':
				OPEN(i); Ct; Cb; CLOSE();
				break;

			case 'd':
				OPEN(i); LL; Dt; Db; CLOSE();
				break;
				
			case 'e':
				OPEN(i); LL; TT; Em; BB; CLOSE();
				break;

			case 'f':
				OPEN(i); LL; TT; Em; CLOSE();
				break;

			case 'g':
				OPEN(i); TT; LL; BB; Hbr; Gm; CLOSE();
				break;

			case 'h':
				OPEN(i); LL; MM; RR; CLOSE();
				break;

			case 'i':
				OPEN(i); TT; II; BB; CLOSE();
				break;

			case 'j':
				OPEN(i); TT; II; Jb; CLOSE();
				break;

			case 'k':
				OPEN(i); LL; Ku; Kd; CLOSE();
				break;

			case 'l':
				OPEN(i); LL; BB; CLOSE();
				break;

			case 'm':
				OPEN(i); LL; Md; Mu; RR; CLOSE();
				break;

			case 'n':
				OPEN(i); LL; Xl; RR; CLOSE();
				break;

			case 'o':
				OPEN(i); Otl; Otr; Obl; Obr; CLOSE();
				break;

			case 'p':
				OPEN(i); LL; Bt; Bi; CLOSE();
				break;

			case 'q':
				OPEN(i); Otl; Otr; Obl; Obr; Qt; CLOSE();
				break;

			case 'r':
				OPEN(i); LL; Bt; Bi; Kd; CLOSE();
				break;

			case 's':
				OPEN(i); Otl; St; MM; Obr; Sb; CLOSE();
				break;

			case 't':
				OPEN(i); TT; II; CLOSE();
				break;

			case 'u':
				OPEN(i); LL; BB; RR; CLOSE();
				break;

			case 'v':
				OPEN(i); Vl; Vr; CLOSE();
				break;

			case 'w':
				OPEN(i); LL; Wu; Wd; RR; CLOSE();
				break;

			case 'x':
				OPEN(i); Xl; Xr; CLOSE();
				break;

			case 'y':
				OPEN(i); Yl; Yb; Yr; CLOSE();
				break;

			case 'z':
				OPEN(i); TT; Xr; BB; CLOSE();
				break;


			case '+':
				OPEN(i);
					PT(25,50);PT(75,50);
					PT(50,25);PT(50,75);
				CLOSE();
				break;
			case '-':
				OPEN(i); PT(25,50);PT(75,50); CLOSE();
				break;
			case '.':
				OPEN(i); PT(50,0);PT(50,10); CLOSE();
				break;
			case '=':
				OPEN(i);
				PT(25,25);PT(75,25);
				PT(25,75);PT(75,75);
				CLOSE();
				break;


			case '0':
				OPEN(i); LL; TT; RR; BB; Xr; CLOSE();
				break;

			case '1':
				OPEN(i); II; CLOSE();
				break;

			case '2':
				OPEN(i); TT; Htr; MM; Hbl; BB; CLOSE();
				break;

			case '3':
				OPEN(i); TT; RR; MM; BB; CLOSE();
				break;

			case '4':
				OPEN(i); Htl; MM; RR; CLOSE();
				break;

			case '5':
				OPEN(i); TT; Htl; MM; Hbr; BB; CLOSE();
				break;

			case '6':
				OPEN(i); TT; Htl; MM; Hbr; BB; Hbl; CLOSE();
				break;

			case '7':
				OPEN(i); TT; RR; CLOSE();
				break;

			case '8':
				OPEN(i); TT; BB; LL; RR; MM; CLOSE();
				break;

			case '9':
				OPEN(i); TT; Htl; MM; RR; BB; CLOSE();
				break;
		}
	}
}

void glutTextPrint (int x, int y, char *msg)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(x,y,0);

	for (; *msg; msg++)
	{
		int i = FONT + tolower(*msg);
		glCallList(i);
		glTranslatef(Text_w * 1.25,0,0);
	}
}
