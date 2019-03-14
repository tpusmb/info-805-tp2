/**
@file Sphere.cpp
*/
#include <cmath>
#include "Sphere.h"

void
rt::Sphere::draw(Viewer & /* viewer */ ) {
    Material m = material;
    // Taking care of south pole
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(m.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, m.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, m.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, m.shinyness);
    Point3 south_pole = localize(-90, 0);
    glNormal3fv(getNormal(south_pole));
    glVertex3fv(south_pole);
    for (int x = 0; x <= NLON; ++x) {
        Point3 p = localize(-90 + 180 / NLAT, x * 360 / NLON);
        glNormal3fv(getNormal(p));
        glVertex3fv(p);
    }
    glEnd();
    // Taking care of in-between poles
    for (int y = 1; y < NLAT - 1; ++y) {
        glBegin(GL_QUAD_STRIP);
        glColor4fv(m.ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, m.diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, m.specular);
        glMaterialf(GL_FRONT, GL_SHININESS, m.shinyness);
        for (int x = 0; x <= NLON; ++x) {
            Point3 p = localize(-90 + y * 180 / NLAT, x * 360 / NLON);
            Point3 q = localize(-90 + (y + 1) * 180 / NLAT, x * 360 / NLON);
            glNormal3fv(getNormal(p));
            glVertex3fv(p);
            glNormal3fv(getNormal(q));
            glVertex3fv(q);
        }
        glEnd();
    }
    // Taking care of north pole
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(m.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, m.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, m.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, m.shinyness);
    Point3 north_pole = localize(90, 0);
    glNormal3fv(getNormal(north_pole));
    glVertex3fv(north_pole);
    for (int x = NLON; x >= 0; --x) {
        Point3 p = localize(-90 + (NLAT - 1) * 180 / NLAT, x * 360 / NLON);
        glNormal3fv(getNormal(p));
        glVertex3fv(p);
    }
    glEnd();
}

rt::Point3
rt::Sphere::localize(Real latitude, Real longitude) const {
    static const Real conv_deg_rad = 2.0 * M_PI / 360.0;
    latitude *= conv_deg_rad;
    longitude *= conv_deg_rad;
    return center
           + radius * Point3(cos(longitude) * cos(latitude),
                             sin(longitude) * cos(latitude),
                             sin(latitude));
}

rt::Vector3
rt::Sphere::getNormal(Point3 p) {
    Vector3 u = p - center;
    Real l2 = u.dot(u);
    if (l2 != 0.0) u /= sqrt(l2);
    return u;
}

rt::Material
rt::Sphere::getMaterial(Point3 /* p */) {
    return material; // the material is constant along the sphere.
}

rt::Real
rt::Sphere::rayIntersection(const Ray &ray, Point3 &p) {
    Vector3 center_direction = ray.origin - center;
    Vector3 center_closest_point = (center_direction) - (center_direction).dot(ray.direction) * ray.direction;
    Real distance_centre_pow_2 = center_closest_point.dot(center_closest_point);
    Real sphere_distance = distance_centre_pow_2 - radius * radius;

    // if there is a intersection 
    if (sphere_distance <= 0) {
        // Solve equation
        Real delta = 4 * (ray.direction.dot(center_direction) * ray.direction.dot(center_direction)) -
                     4 * ((center_direction).dot(center_direction) - radius * radius);
        // get the tow solutions
        Real solution1 = (-2 * ray.direction.dot(center_direction) - (float) sqrt(delta)) / 2;
        Real solution2 = (-2 * ray.direction.dot(center_direction) + (float) sqrt(delta)) / 2;

        // Closes intersection 
        Real solution;
        // case 1: The object in front of the ray
        if (solution1 >= 0 && solution2 >= 0) {
            // get closes solution
            solution = std::min(solution1, solution2);
            // cse 2: Object back from the ray (no intersection)
        } else if (solution1 < 0 && solution2 < 0) {
            sphere_distance = -sphere_distance;
            // The closes point is the ray origin
            solution = 0;
            // case 3: 2 different signed. The rays are in the ball
        } else {
            // We get the closes one
            solution = std::max(solution1, solution2);
        }
        // get the intersection point
        p = ray.origin + solution * ray.direction;

    } else {
        p = center - center_closest_point;
    }
    return sphere_distance;
}