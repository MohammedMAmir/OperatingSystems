#include <assert.h>
#include "common.h"
#include "point.h"
#include "math.h"

void
point_translate(struct point *p, double x, double y)
{
    p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double distance, x_distance, y_distance;
	
	x_distance = p1->x - p2->x;
	y_distance = p1->y - p2->y;
	
	distance = sqrt(x_distance*x_distance + y_distance*y_distance);
	return distance;
}


int
point_compare(const struct point *p1, const struct point *p2)
{
	struct point origin;
	point_set(&origin, 0.0, 0.0);
	
	double d1=point_distance(&origin, p1);
	double d2=point_distance(&origin, p2);
	
	if(d1 < d2){
	    return -1;
	}else if(d1 == d2){
	    return 0;
	}else if(d1 > d2){
	    return 1;
	}    
	return 0;
}
