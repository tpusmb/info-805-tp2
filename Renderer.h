/**
@file Renderer.h
@author JOL
*/
#pragma once
#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Color.h"
#include "Image2D.h"
#include "Ray.h"
#include <math.h>

/// Namespace RayTracer
namespace rt {

    inline void progressBar(std::ostream &output,
                            const double currentValue, const double maximumValue) {
        static const int PROGRESSBARWIDTH = 60;
        static int myProgressBarRotation = 0;
        static int myProgressBarCurrent = 0;
        // how wide you want the progress meter to be
        double fraction = currentValue / maximumValue;

        // part of the progressmeter that's already "full"
        int dotz = static_cast<int>(floor(fraction * PROGRESSBARWIDTH));
        if (dotz > PROGRESSBARWIDTH) dotz = PROGRESSBARWIDTH;

        // if the fullness hasn't changed skip display
        if (dotz == myProgressBarCurrent) return;
        myProgressBarCurrent = dotz;
        myProgressBarRotation++;

        // create the "meter"
        int ii = 0;
        output << "[";
        // part  that's full already
        for (; ii < dotz; ii++) output << "#";
        // remaining part (spaces)
        for (; ii < PROGRESSBARWIDTH; ii++) output << " ";
        static const char *rotation_string = "|\\-/";
        myProgressBarRotation %= 4;
        output << "] " << rotation_string[myProgressBarRotation]
               << " " << (int) (fraction * 100) << "/100\r";
        output.flush();
    }

    struct Background {
        virtual Color backgroundColor(const Ray &ray) = 0;
    };

    struct BasicBackground : public Background {
        Color backgroundColor(const Ray &ray) {
            Color result = Color(0.0f, 0.0f, 0.0f);
            if (ray.direction[2] >= 0 && ray.direction[2] <= 1.0) {
                return Color(1.0f, 1.0f, 1.0f) + ray.direction[2] * (Color(0.0f, 0.0f, 1.0f) - Color(1.0f, 1.0f, 1.0f));
            } else {
                Real x = -0.5f * ray.direction[0] / ray.direction[2];
                Real y = -0.5f * ray.direction[1] / ray.direction[2];
                Real d = sqrt(x * x + y * y);
                Real t = std::min(d, 30.0f) / 30.0f;
                x -= floor(x);
                y -= floor(y);
                if (((x >= 0.5f) && (y >= 0.5f)) || ((x < 0.5f) && (y < 0.5f)))
                    result += (1.0f - t) * Color(0.2f, 0.2f, 0.2f) + t * Color(1.0f, 1.0f, 1.0f);
                else
                    result += (1.0f - t) * Color(0.4f, 0.4f, 0.4f) + t * Color(1.0f, 1.0f, 1.0f);
            }
            return result;
        }
    };

    /// This structure takes care of rendering a scene.
    struct Renderer {

        /// The scene to render
        Scene *ptrScene;
        /// The origin of the camera in space.
        Point3 myOrigin;
        /// (myOrigin, myOrigin+myDirUL) forms a ray going through the upper-left
        /// corner pixel of the viewport, i.e. pixel (0,0)
        Vector3 myDirUL;
        /// (myOrigin, myOrigin+myDirUR) forms a ray going through the upper-right
        /// corner pixel of the viewport, i.e. pixel (width,0)
        Vector3 myDirUR;
        /// (myOrigin, myOrigin+myDirLL) forms a ray going through the lower-left
        /// corner pixel of the viewport, i.e. pixel (0,height)
        Vector3 myDirLL;
        /// (myOrigin, myOrigin+myDirLR) forms a ray going through the lower-right
        /// corner pixel of the viewport, i.e. pixel (width,height)
        Vector3 myDirLR;
        // On rajoute un pointeur vers un objet Background
        Background *ptrBackground = new BasicBackground();

        int myWidth;
        int myHeight;

        Renderer() : ptrScene(0) {}

        Renderer(Scene &scene) : ptrScene(&scene) {}

        void setScene(rt::Scene &aScene) { ptrScene = &aScene; }

        void setViewBox(Point3 origin,
                        Vector3 dirUL, Vector3 dirUR, Vector3 dirLL, Vector3 dirLR) {
            myOrigin = origin;
            myDirUL = dirUL;
            myDirUR = dirUR;
            myDirLL = dirLL;
            myDirLR = dirLR;
        }

        void setResolution(int width, int height) {
            myWidth = width;
            myHeight = height;
        }


        /// The main rendering routine
        void render(Image2D<Color> &image, int max_depth) {
            std::cout << "Rendering into image ... might take a while." << std::endl;
            image = Image2D<Color>(myWidth, myHeight);
            for (int y = 0; y < myHeight; ++y) {
                Real ty = (Real) y / (Real) (myHeight - 1);
                progressBar(std::cout, ty, 1.0);
                Vector3 dirL = (1.0f - ty) * myDirUL + ty * myDirLL;
                Vector3 dirR = (1.0f - ty) * myDirUR + ty * myDirLR;
                dirL /= dirL.norm();
                dirR /= dirR.norm();
                for (int x = 0; x < myWidth; ++x) {
                    Real tx = (Real) x / (Real) (myWidth - 1);
                    Vector3 dir = (1.0f - tx) * dirL + tx * dirR;
                    Ray eye_ray = Ray(myOrigin, dir, max_depth);
                    Color result = trace(eye_ray);
                    image.at(x, y) = result.clamp();
                }
            }
            std::cout << "Done." << std::endl;
        }


        // Affiche les sources de lumières avant d'appeler la fonction qui
        // donne la couleur de fond.
        Color background(const Ray &ray) {
            Color result = Color(0.0, 0.0, 0.0);
            for (Light *light : ptrScene->myLights) {
                Real cos_a = light->direction(ray.origin).dot(ray.direction);
                if (cos_a > 0.99f) {
                    Real a = acos(cos_a) * 360.0 / M_PI / 8.0;
                    a = std::max(1.0f - a, 0.0f);
                    result += light->color(ray.origin) * a * a;
                }
            }
            if (ptrBackground != 0) result += ptrBackground->backgroundColor(ray);
            return result;
        }

        /// The rendering routine for one ray.
        /// @return the color for the given ray.
        Color trace(const Ray &ray) {
            assert(ptrScene != 0);
            Color result = Color(0.0, 0.0, 0.0);
            GraphicalObject *obj_i = 0; // pointer to intersected object
            Point3 p_i;       // point of intersection

            // Look for intersection in this direction.
            Real ri = ptrScene->rayIntersection(ray, obj_i, p_i);
            // Nothing was intersected
            if (ri >= 0.0f) return background(ray); // some background color
            if (ray.depth > 0 && obj_i->getMaterial(p_i).coef_reflexion != 0) {
                Ray ray_reflect(ray.origin, reflect(ray.direction, obj_i->getNormal(p_i)));
                ray_reflect.depth--;
                Color color_reflect = trace(ray_reflect);
                result += color_reflect * obj_i->getMaterial(p_i).specular * obj_i->getMaterial(p_i).coef_reflexion;
            }
            if (ray.depth > 0 && obj_i->getMaterial(p_i).coef_refraction != 0) {
                Ray ray_refract = refractionRay(ray, p_i, obj_i->getNormal(p_i), obj_i->getMaterial(p_i));
                ray_refract.depth--;
                Color color_refract = trace(ray_refract);
                result += color_refract * obj_i->getMaterial(p_i).diffuse * obj_i->getMaterial(p_i).coef_refraction;
            }
            if(ray.depth > 0)
                result += illumination(ray, obj_i, p_i) * obj_i->getMaterial(p_i).coef_diffusion;
            else
                result += illumination(ray, obj_i, p_i);
            return result;
        }

        /// Calcule l'illumination de l'objet obj au point p, sachant que l'observateur est le rayon ray.
        Color illumination(const Ray &ray, GraphicalObject *obj, Point3 p) {
            Color result = Color(0.0, 0.0, 0.0);
            Color temp_light_color;
            // Get all light source
            for (auto &light : ptrScene->myLights) {
                temp_light_color = light->color(p);
                temp_light_color = shadow(Ray(p, light->direction(p)), temp_light_color);
                // get the diffusion diffusion_coefficient base on the Phong model
                Real diffusion_coefficient = light->direction(p).dot(obj->getNormal(p));
                if (diffusion_coefficient < 0) diffusion_coefficient = 0;
                result += diffusion_coefficient * obj->getMaterial(p).diffuse * temp_light_color;

                // get the specular color base on the Phong model
                Vector3 reflect_vector = reflect(ray.direction, obj->getNormal(p));
                Real specular_component = light->direction(p).dot(reflect_vector);
                if (specular_component >= 0) {
                    specular_component = powf(specular_component, obj->getMaterial(p).shinyness);
                    result += specular_component * obj->getMaterial(p).specular * temp_light_color;
                }
            }
            // add the ambiance color
            result += obj->getMaterial(p).ambient;

            return result;
        }

        /// Calcule le vecteur réfléchi à W selon la normale N.
        Vector3 reflect(const Vector3 &W, Vector3 N) const {
            return W - 2 * W.dot(N) * N;
        }

        /// Calcule le rayon réfracté
        Ray refractionRay(const Ray &aRay, const Point3 &p, Vector3 N, const Material &m) {
            Real tmp;
            Real r = m.in_refractive_index / m.out_refractive_index;
            Real c = (-1.0f) * N.dot(aRay.direction);
            //When the ray is inside the object and go out

            if (aRay.direction.dot(N) <= 0) {
                r = m.out_refractive_index / m.in_refractive_index;
            }
            Real alpha = 1 - (r * r) * (1 - (c * c));
            if (c > 0)
                tmp = r * c - sqrt(alpha);
            else {
                tmp = r * c + sqrt(alpha);
            }

            Vector3 vRefrac = Vector3(r * aRay.direction + tmp * N);

            //Total reflexion
            if (alpha < 0) {
                vRefrac = reflect(aRay.direction, N);
            }

            Ray newRay = Ray(p + vRefrac * 0.01f, vRefrac, aRay.depth - 1);

            return newRay;
        }

        /// Calcule la couleur de la lumière (donnée par light_color) dans la
        /// direction donnée par le rayon. Si aucun objet n'est traversé,
        /// retourne light_color, sinon si un des objets traversés est opaque,
        /// retourne du noir, et enfin si les objets traversés sont
        /// transparents, attenue la couleur.
        Color shadow(const Ray &ray, Color light_color) {
            Color c = light_color;
            Ray p_ray = ray;
            GraphicalObject *obj = 0; // pointer to intersected object
            Point3 intersection_point;       // point of intersection
            while (c.max() > 0.003f) {
                p_ray.origin += ray.direction;
                Real ri = ptrScene->rayIntersection(p_ray, obj, intersection_point);
                // intersection
                if (ri < 0.0f) {
                    c = c * obj->getMaterial(intersection_point).coef_refraction *
                        obj->getMaterial(intersection_point).diffuse;
                    p_ray.origin = intersection_point;
                } else {
                    break;
                }
            }
            return c;
        }
    };

} // namespace rt

#endif // #define _RENDERER_H_
