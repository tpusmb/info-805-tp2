#include <qapplication.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include "Viewer.h"
#include "Scene.h"
#include "Sphere.h"
#include "Material.h"
#include "PointLight.h"

using namespace std;
using namespace rt;

void addBubble(Scene &scene, Point3 c, Real r, Material transp_m) {
    Material revert_m = transp_m;
    std::swap(revert_m.in_refractive_index, revert_m.out_refractive_index);
    Sphere *sphere_out = new Sphere(c, r, transp_m);
    Sphere *sphere_in = new Sphere(c, r - 0.02f, revert_m);
    scene.addObject(sphere_out);
    scene.addObject(sphere_in);
}

float to_rad(float degres) {
    return degres * (M_PI / 180);
}

int main(int argc, char **argv) {
    // Read command lines arguments.
    QApplication application(argc, argv);

    // Creates a 3D scene
    Scene scene;

    // Light at infinity
    Light *light0 = new PointLight(GL_LIGHT0, Point4(1, 1, 1, 0),
                                   Color(1.0, 1.0, 1.0));
    Light *light1 = new PointLight(GL_LIGHT1, Point4(10, 10, 10, 1), Color(1.0, 0.0, 1.0));
    scene.addLight(light0);
    scene.addLight(light1);
// Objects
//Sphere *sphere3 = new Sphere(Point3(10, 10, 5), 3.0, Material::whitePlastic());
 //   scene.addObject(sphere3);

    int center = 10;
    int radius = 20;
    int delta_angle = round(360 / radius);
    int x, y, z;
    z = 5;
    while (radius > -40) {
        for (int incre_angle = 0; incre_angle < 360; incre_angle += delta_angle) {
            x = round(center + radius * sin(to_rad(incre_angle)));
            y = round(center + radius * cos(to_rad(incre_angle)));
            addBubble(scene, Point3(x, y, z), 2.0, Material::glass());
        }
        radius -= 5;
        if(radius == 0)
            radius = -5;
        z += 4;
        delta_angle = round(360 / abs(radius));
    }




    // Instantiate the viewer.
    Viewer viewer;
    // Give a name
    viewer.setWindowTitle("Ray-tracer preview");

    // Sets the scene
    viewer.setScene(scene);

    // Make the viewer window visible on screen.
    viewer.show();
    // Run main loop.
    application.exec();
    return 0;
}



