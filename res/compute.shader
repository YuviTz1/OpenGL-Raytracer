#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen;
layout(std140, binding = 1) uniform AccumulationBlock
{
    uint frameCount;
};
layout(rgba32f, binding = 1) uniform image2D accumulationImage;
uniform float uBackgroundStrength;


const float MIN_DIST = 0.0001;
const float MAX_DIST = 1000.0;
//const int MAX_SPHERES = 4;

const int MATERIAL_DIFFUSE = 0;
const int MATERIAL_METAL = 1;
const int MATERIAL_GLASS = 2;
const int MATERIAL_LIGHT = 3;


//Camera uniform
layout(std140, binding = 0) uniform cameraBlock
{
    vec4 cameraPos;
    vec4 cameraFront;
    vec4 cameraUp;
    vec4 cameraRight;
    vec2 fovAndAspect;
    vec2 padding;
};

struct Material 
{
    int type;
    int _padA[3];
    vec4 albedo;
    vec4 emission;
    float roughness;
    float ior;
    float _padB[2];
};

struct Sphere
{
    vec4 position;
    float radius;
    int _padC[3];
    Material material;
};

layout(std430, binding = 1) buffer SphereBuffer
{
    Sphere spheres[];
};

struct Triangle {
    vec4 v0, v1, v2;
    vec4 n0, n1, n2;
};

struct Mesh {
    Triangle triangles[1024];
    Material material;
    int triangleCount;   // Number of triangles in the mesh
    int _padD[3]; 
};

layout(std430, binding = 2) readonly buffer MeshBuffer {
    Mesh meshes[];
};

uniform int uSphereCount;
uniform int uMeshCount; // NEW: number of meshes

struct Ray
{
    vec3 origin;
    vec3 direction;
};

//determine point on ray at parameter t
vec3 ray_at(float t, Ray ray) 
{
    return ray.origin + t*ray.direction;
}

Ray createCameraRay(vec2 uv)
{
    //Convert UV from [0,1] to [-1,1] and apply aspect ratio correction
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.x *= fovAndAspect.y;
    float tanFov = tan(fovAndAspect.x * 0.5);

    //Create camera space vectors
    vec3 rayDir = normalize(
        cameraFront.xyz +
        ndc.x * tanFov * cameraRight.xyz +
        ndc.y * tanFov * cameraUp.xyz
    );

    return Ray(cameraPos.xyz, rayDir);
}

struct HitRecord
{
    vec3 point;
    vec3 normal;
    float t;
    bool front_face;
    Material material;
};

uint seed;
uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float random_float()
{
    seed = wang_hash(seed);
    return float(seed) / 4294967296.0;
}

float random_float(float min, float max)
{
    return float(min + (max - min) * random_float());
}

vec3 random_vector()
{
    return vec3(random_float(), random_float(), random_float());
}

vec3 random_vector(float min, float max)
{
    return vec3(random_float(min, max), random_float(min, max), random_float(min, max));
}

vec3 random_unit_vector()
{
    while (true)
    {
        vec3 p = random_vector(-1.0, 1.0);
        float length_squared = dot(p, p);
        if ( 1e-160 <=length_squared && length_squared <= 1.0)
        {
            return p/sqrt(length_squared);
        }
    }
}

vec3 random_on_hemisphere(vec3 normal)
{
    vec3 on_unit_sphere = random_unit_vector();
    if (dot(on_unit_sphere, normal) > 0.0)
    {
        return on_unit_sphere;
    }
    else
    {
        return -on_unit_sphere;
    }
}

vec2 random_in_unit_square()
{
    return vec2(random_float(), random_float());
}

vec2 get_subpixel_offset(int sampleIdx)
{
    int x = sampleIdx % 2;
    int y = sampleIdx / 2;

    vec2 stratifiedPos = vec2(x, y) * 0.5;
    vec2 jitter = random_in_unit_square() * 0.5;

    return stratifiedPos + jitter;
}

vec3 linear_to_gamma(vec3 color)
{
    if(color.r < 0.0 || color.g < 0.0 || color.b < 0.0)
    {
        return vec3(0.0);
    }
    return vec3(sqrt(color.r), sqrt(color.g), sqrt(color.b));
}

//reflect function for metals
vec3 reflect(vec3 v, vec3 n)
{
    return v - 2.0 * dot(v, n) * n;
}

vec3 refract(vec3 uv, vec3 n, float etai_over_etat)
{
    float cos_theta = min(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(abs(1.0 - dot(r_out_perp, r_out_perp))) * n;
    return r_out_parallel + r_out_perp;
}

float schlick(float cos, float ref_idx)
{
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cos), 5.0);
}

bool hit_sphere(Ray ray, Sphere sphere, out HitRecord rec)
{
    vec3 oc = ray.origin - sphere.position.xyz;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4.0 * a * c;
    
    if(discriminant < 0.0)    //no roots
    {
        return false;
    }

    float root = (-b - sqrt(discriminant)) / (2.0 * a);
    if (root < MIN_DIST || root > MAX_DIST)    //roots are outside of the range
    {
        root = (-b + sqrt(discriminant)) / (2.0 * a);
        if (root < MIN_DIST || root > MAX_DIST)
        {
            return false;
        }
    }
    rec.t = root;
    rec.point = ray_at(rec.t, ray);
    vec3 outward_normal = normalize(rec.point - sphere.position.xyz);    //normal from the sphere center to the hit point

    rec.front_face = dot(ray.direction, outward_normal) < 0.0;
    rec.normal = normalize(rec.front_face ? outward_normal : -outward_normal);
    rec.material = sphere.material;

    return true;
}

// NEW: Ray-triangle intersection (Möller–Trumbore)
bool hit_triangle(Ray ray, Mesh mesh, Triangle tri, out HitRecord rec)
{
    vec3 v0 = tri.v0.xyz;
    vec3 v1 = tri.v1.xyz;
    vec3 v2 = tri.v2.xyz;

    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;

    vec3 pvec = cross(ray.direction, e2);
    float det = dot(e1, pvec);

    // Accept both sides, reject near-parallel
    const float EPS = 1e-8;
    if (abs(det) < EPS) return false;

    float invDet = 1.0 / det;
    vec3 tvec = ray.origin - v0;

    float u = dot(tvec, pvec) * invDet;
    if (u < 0.0 || u > 1.0) return false;

    vec3 qvec = cross(tvec, e1);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < 0.0 || (u + v) > 1.0) return false;

    float t = dot(e2, qvec) * invDet;
    if (t < MIN_DIST || t > MAX_DIST) return false;

    rec.t = t;
    rec.point = ray_at(t, ray);

    // Interpolated normal if provided, otherwise geometric
    float w = 1.0 - u - v;
    vec3 shadingN = tri.n0.xyz * w + tri.n1.xyz * u + tri.n2.xyz * v;

    if (dot(shadingN, shadingN) < 1e-12)
    {
        shadingN = normalize(cross(e1, e2));
    }
    else
    {
        shadingN = normalize(shadingN);
    }

    rec.front_face = dot(ray.direction, shadingN) < 0.0;
    rec.normal = rec.front_face ? shadingN : -shadingN;
    rec.material = mesh.material;

    return true;
}

bool scatter(Ray ray, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    if(rec.material.type == MATERIAL_DIFFUSE)
    {
        vec3 scatter_direction = rec.normal + random_unit_vector();
        scattered.origin = rec.point;
        scattered.direction = normalize(scatter_direction);

        scattered = Ray(scattered.origin, scattered.direction);
        attenuation = rec.material.albedo.xyz;

        return true;
    }
    if(rec.material.type == MATERIAL_METAL)
    {
        vec3 reflected = reflect(normalize(ray.direction), rec.normal);
        scattered.origin = rec.point;
        scattered.direction = normalize(reflected + rec.material.roughness * random_on_hemisphere(rec.normal));

        scattered = Ray(scattered.origin, scattered.direction);
        attenuation = rec.material.albedo.xyz;

        return (dot(scattered.direction, rec.normal) > 0.0);
    }
    if(rec.material.type == MATERIAL_GLASS)
    {
        attenuation = vec3(1.0);
        float etai_over_etat = rec.front_face ? (1.0 / rec.material.ior) : rec.material.ior;    //1.5 being refraction index
        vec3 unit_direction = normalize(ray.direction);
        float cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
        float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        bool cannot_refract = etai_over_etat * sin_theta > 1.0;
        vec3 direction;
        if (cannot_refract || schlick(cos_theta, etai_over_etat) > random_float())
        {
            direction = reflect(unit_direction, rec.normal);
        }
        else
        {
            direction = refract(unit_direction, rec.normal, etai_over_etat);
        }
        scattered.origin = rec.point;
        scattered.direction = normalize(direction);
        scattered = Ray(scattered.origin, scattered.direction);
        return true;
    }
    return false;
}

// Sky gradient
vec3 sky_color(vec3 dir)
{
    vec3 d = normalize(dir);
    float t = clamp(0.5 * (d.y + 1.0), 0.0, 1.0); // 0 at horizon looking down, 1 at zenith
    vec3 horizon = vec3(0.94, 0.97, 1.00); // near-white horizon
    vec3 zenith  = vec3(0.529, 0.808, 0.922); // sky blue
    float smoothT = pow(t, 0.65);
    return mix(horizon, zenith, smoothT) * uBackgroundStrength; // final multiplier can be adjusted for brightness
}

vec3 ray_color(Ray ray, int spheres_count)
{
    vec3 accumulated_color = vec3(1.0); // Path throughput
    int max_bounces = 5;
    vec3 background = vec3(0.01, 0.01, 0.01); // Very faint background light

    for (int bounce = 0; bounce < max_bounces; bounce++)
    {
        HitRecord closest_rec;
        float closest_t = MAX_DIST; // A large value to represent infinity
        bool hit_anything = false;

        // Spheres
        for (int i = 0; i < spheres_count; i++)
        {
            HitRecord temp_rec;
            if (hit_sphere(ray, spheres[i], temp_rec) && temp_rec.t < closest_t)
            {
                hit_anything = true;
                closest_t = temp_rec.t;
                closest_rec = temp_rec;
            }
        }

        // Triangles
        for (int i = 0; i < uMeshCount; i++)
        {
            for(int j=0;j<meshes[i].triangleCount;j++)
            {
                HitRecord temp_rec;
                if (hit_triangle(ray, meshes[i], meshes[i].triangles[j], temp_rec) && temp_rec.t < closest_t)
                {
                    hit_anything = true;
                    closest_t = temp_rec.t;
                    closest_rec = temp_rec;
                }
            }
        }

        if (hit_anything)
        {
            // If hit a light, return its emission and terminate
            if (closest_rec.material.type == MATERIAL_LIGHT)
            {
                return accumulated_color * closest_rec.material.emission.xyz;
            }

            Ray scattered;
            vec3 attenuation;
            if (scatter(ray, closest_rec, attenuation, scattered))
            {
                accumulated_color *= attenuation; // Accumulate color
                ray = scattered;         // Update ray for the next bounce

                if(max(max(attenuation.r, attenuation.g), attenuation.b) < 0.01)
                {
                    return vec3(0.0); // If attenuation is too low, return black
                }
            }
            else
            {
                return vec3(0.0); // No scattering, return black
            }
        }
        else
        {
            // No hit: return faint background
            return accumulated_color * sky_color(ray.direction);
        }
    }

    // Max bounces reached: return faint background
    return accumulated_color * sky_color(ray.direction);
}

void main()
{
    vec4 pixel = vec4(0.075, 0.133, 0.173, 1.0);        //random pixel colors; redundant
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    // initialize the seed with the pixel coordinates for jitter free image
    seed = uint(pixel_coords.x ^ pixel_coords.y ^ frameCount ^ uint(gl_GlobalInvocationID.x * 1973 + gl_GlobalInvocationID.y * 9277));
    
    ivec2 dims = imageSize(screen);
    float aspect = float(dims.x) / float(dims.y);

    int samples_per_pixel = 5;
    vec3 accumulated_color = vec3(0.0);

    for (int i=0; i<samples_per_pixel; i++)
    {
        vec2 offset = get_subpixel_offset(i);
        vec2 uv = (vec2(pixel_coords) + offset) / vec2(dims);
        Ray ray = createCameraRay(uv);

        accumulated_color += ray_color(ray, uSphereCount);
    }

    pixel = vec4(linear_to_gamma(accumulated_color / float(samples_per_pixel)), 1.0);

    // Temporal accumulation
    vec4 accumulatedColor = imageLoad(accumulationImage, pixel_coords);
    vec4 finalColor;    

    if (frameCount == 0)
    {
        finalColor = pixel;
    }
    else
    {
        float weight = 1.0 / float(frameCount + 1);
        finalColor = mix(accumulatedColor, pixel, weight);
    }

    // Store results
    imageStore(accumulationImage, pixel_coords, finalColor);
    //imageStore(imgOutput, pixel, vec4(finalColor, 1.0));

    imageStore(screen, pixel_coords, finalColor);
}