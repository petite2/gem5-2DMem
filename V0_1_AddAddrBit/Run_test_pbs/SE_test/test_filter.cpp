#include <iostream>
#include<memory.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<malloc.h>

int main(int argc,char *argv[])
{
		double *a,*b;
		int loop=atoi(argv[1]);
		int co=atoi(argv[2]);
		int width,height;
		width=height=atoi(argv[3]);
		//width=height=16;
		int sw=8,sh=8;
		int size=width*height;
		//a=(double *)malloc(sizeof(double)*size);
    //b=(double *)malloc(sizeof(double)*size);
		a=(double *)memalign(getpagesize(),sizeof(double)*size);
  	b=(double *)memalign(getpagesize(),sizeof(double)*size);

		for(int l=0;l<loop;l++)
		{
		    //1:write by column  
	    	if(co==1)
				{
	     			for(int i=0;i<width;i++)
	         	{
	         			for(int j=0;j<height;j++)
	         			{
	         					a[j*width+i]=j*width+i;
	         			}
	         	}
	     			
             
	         	//read by column
	         	for(int i=0;i<size;i++)
						printf("%f ",a[i]);
				
				}
       
	      //2:filter
				if(co==2)
				{

						/*
						for(int i=0;i<width;i++)
	         	{
	         			for(int j=0;j<height;j++)
	         			{
	         					a[j*width+i]=j*width+i;
	         			}
	         	}*/
						//add padding
	        	for(int w=0;w<width;w++)
	        	{
								int h=2;
	        		  a[(h-2)*width+w]=a[h*width+w]*2 - a[(h+1)*width+w]*4 - a[(h+2)*width+w] + a[(h+3)*width+w];
	        		  a[(h-1)*width+w]=a[h*width+w]*0 + a[(h+1)*width+w]*2 + a[(h+2)*width+w]*0 - a[(h+3)*width+w];

								h=height-6;
	        		  a[(h+4)*width+w]=a[h*width+w]*0 + a[(h+1)*width+w]*2 + a[(h+2)*width+w]*0 - a[(h+3)*width+w];
	        		  a[(h+5)*width+w]=a[h*width+w]*2 - a[(h+1)*width+w]*4 - a[(h+2)*width+w] + a[(h+3)*width+w];
	        	}
	        	//filter_vertical
	        	for(int w=0;w<width;w++)
	        	{
	        			for(int h=0;h<height-4;h++)
	        			{
	        					b[(h+2)*width+w]=(a[h*width+w] + a[(h+1)*width+w]*4 + a[(h+2)*width+w]*6 + a[(h+3)*width+w]*4 + a[(h+4)*width+w] +8)/4;
	        			}
	        	}

						for(int i=0;i<size;i++)
								printf("%f ",b[i]);
				
				}

	    	//3:border (mix)
				if(co==3)
				{

						/*for(int i=0;i<width;i++)
	         	{
	         			for(int j=0;j<height;j++)
	         			{
	         					a[j*width+i]=j*width+i;
	         			}
	         	}*/
				
	        	for(int i=0;i<sw-1;i++)
	        	{
	        			b[i*4]=a[i]*1+a[i+1]*0;
			    			b[i*4+1]=a[i]*3/4.00+a[i+1]*1/4.00;
	            	b[i*4+2]=a[i]*2/4.00+a[i+1]*2/4.00;
	        	    b[i*4+3]=a[i]*1/4.00+a[i+1]*3/4.00;

			    			b[(sh*4-1)*sh*4+i*4]=  a[(sh-1)*sw+i]*1+  a[(sh-1)*sw+i+1]*0;
			    			b[(sh*4-1)*sh*4+i*4+1]=a[(sh-1)*sw+i]*3/4.00+a[(sh-1)*sw+i+1]*1/4.00;
	            	b[(sh*4-1)*sh*4+i*4+2]=a[(sh-1)*sw+i]*2/4.00+a[(sh-1)*sw+i+1]*2/4.00;
	        	    b[(sh*4-1)*sh*4+i*4+3]=a[(sh-1)*sw+i]*1/4.00+a[(sh-1)*sw+i+1]*3/4.00;
	        	}

						for(int i=0;i<sh-1;i++)
						{
								b[(i  )*4*sw]=a[i*sw]*1+  a[(i+1)*sw]*0;
			    			b[(i+1)*4*sw]=a[i*sw]*3/4.00+a[(i+1)*sw]*1/4.00;
	            	b[(i+2)*4*sw]=a[i*sw]*2/4.00+a[(i+1)*sw]*2/4.00;
	        	    b[(i+3)*4*sw]=a[i*sw]*1/4.00+a[(i+1)*sw]*3/4.00;

								b[(i  )*4*sw+sw*4-1]=a[i*sw+sw-1]*1+  a[(i+1)*sw+sw-1]*0;
			    			b[(i+1)*4*sw+sw*4-1]=a[i*sw+sw-1]*3/4.00+a[(i+1)*sw+sw-1]*1/4.00;
	            	b[(i+2)*4*sw+sw*4-1]=a[i*sw+sw-1]*2/4.00+a[(i+1)*sw+sw-1]*2/4.00;
	        	    b[(i+3)*4*sw+sw*4-1]=a[i*sw+sw-1]*1/4.00+a[(i+1)*sw+sw-1]*3/4.00;

						}

						for(int i=0;i<sh*sw*4*4;i++)
								printf("%f ",b[i]);
				
				}

				//4:write by row  
	    	if(co==4)
				{
	     			for(int i=0;i<height;i++)
	         	{
	         			for(int j=0;j<width;j++)
	         			{
	         					a[i*width+j]=i*width+j;
	         			}
	         	}
	     			
             
	         	//read by column
	         	for(int i=0;i<size;i++)
								printf("%f ",a[i]);
				
				}

		}
		free(a);
		free(b);
		return 0;
   
}
