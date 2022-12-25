//
// <ctest.h> - collision testing functions
//

#ifndef CTEST_H
#define CTEST_H

#include "main.h"
#include "phyedit.h"

#define MAX_RAY_TESTS           20000

namespace CTest
{
    SINGLE ray_test_time = 0.0;
    SINGLE hit_percentage = 0.0;

    BOOL32 perform_center_directed_ray_test()    
    {
        if (physicsEditor.object.index == INVALID_INSTANCE_INDEX) return FALSE;

        SINGLE hit_count = 0.0;
        Vector intersect;

        Transform xform;
        xform.set_position(physicsEditor.object.center_of_mass);

        S32 dv = physicsEditor.object.scale * 4.0;

        static Vector rnd_s[MAX_RAY_TESTS];
        static Vector rnd_d[MAX_RAY_TESTS];

        for (int i = 0; i < MAX_RAY_TESTS; i++)
        {
            // generate random rays
            
            rnd_s[i].x = (rand() % dv) - (dv / 2);
            rnd_s[i].y = (rand() % dv) - (dv / 2);
            rnd_s[i].z = (rand() % dv) - (dv / 2);
            
            rnd_d[i] = -rnd_s[i];
            rnd_d[i].normalize();
        }


        timer.compute_elapsed_time();

		static Vector normal;

        for (i = 0; i < MAX_RAY_TESTS; i++)
        {
            if (COLLISION->intersect_ray_with_extent(intersect, normal, rnd_s[i], rnd_d[i], *physicsEditor.object.tree, xform))
            {
                hit_count += 1.0;
            }
            
        }

        ray_test_time = timer.compute_elapsed_time() / (SINGLE) MAX_RAY_TESTS;
        hit_percentage = hit_count / (SINGLE) MAX_RAY_TESTS;

        return TRUE;
    }

};

#endif
