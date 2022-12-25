//
// <cvisualize.h> - collision visualization functions
//

#ifndef CVIS_H
#define CVIS_H

#include "main.h"
#include "phyedit.h"

typedef void (* VCallback)(void);

#define CV_TESTS 100

namespace CVisualization
{
    VCallback view_function = NULL;

    // works like a post-render

    void ray_collisions(void)
    {   
        if (physicsEditor.object.index == INVALID_INSTANCE_INDEX) return;

        SINGLE hit_count = 0.0;
        Vector intersect;

        Transform xform;
        xform.set_position(physicsEditor.object.center_of_mass);

        S32 dv = physicsEditor.object.scale * 4.0;

        static Vector rnd_s[CV_TESTS];
        static Vector rnd_d[CV_TESTS];

        for (int i = 0; i < CV_TESTS; i++)
        {
            // generate random rays
            
            rnd_s[i].x = (rand() % dv) - (dv / 2);
            rnd_s[i].y = (rand() % dv) - (dv / 2);
            rnd_s[i].z = (rand() % dv) - (dv / 2);
            
            rnd_d[i] = -rnd_s[i];
            rnd_d[i].normalize();
        }


        timer.compute_elapsed_time();           // messes with fps

        Vector p = ENGINE->get_position(physicsEditor.object.index);
        Matrix R = ENGINE->get_orientation(physicsEditor.object.index);

		Vector normal;

        glDepthFunc(GL_LEQUAL);
        
        glBegin(GL_POINTS);
        glColor4f(1.0, 0.8, 1.0, 1.0);

        for (i = 0; i < CV_TESTS; i++)
        {            
            if (COLLISION->intersect_ray_with_extent(intersect, normal, rnd_s[i], rnd_d[i], *physicsEditor.object.tree, xform))
            {
                intersect = p + (R * intersect);
                glVertex3f(intersect.x, intersect.y, intersect.z);
            }

        }

        glEnd();

    }

}

#endif