#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen;

const float MIN_DIST = 0.0001;
const float MAX_DIST = 1000.0;

const int MATERIAL_DIFFUSE = 0;
const int MATERIAL_METAL = 1;
const int MATERIAL_GLASS = 2;

struct Material 
{
	int type;
	vec3 albedo;
	float roughness;
};

struct Sphere
{
	vec3 position;
	float radius;
};

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

struct HitRecord
{
	vec3 point;
	vec3 normal;
	float t;
	bool front_face;
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

vec3 linear_to_gamma(vec3 color)
{
	if(color.r < 0.0 || color.g < 0.0 || color.b < 0.0)
	{
		return vec3(0.0);
	}
	return vec3(sqrt(color.r), sqrt(color.g), sqrt(color.b));
}

bool hit_sphere(Ray ray, Sphere sphere, out HitRecord rec)
{
	vec3 oc = ray.origin - sphere.position;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float discriminant = b * b - 4.0 * a * c;
	
	if(discriminant < 0.0)	//no roots
	{
		return false;
	}

	float root = (-b - sqrt(discriminant)) / (2.0 * a);
	if (root < MIN_DIST || root > MAX_DIST)	//roots are outside of the range
	{
		root = (-b + sqrt(discriminant)) / (2.0 * a);
		if (root < MIN_DIST || root > MAX_DIST)
		{
			return false;
		}
	}
	rec.t = root;
	rec.point = ray_at(rec.t, ray);
	vec3 outward_normal = normalize(rec.point - sphere.position);	//normal from the sphere center to the hit point

	rec.front_face = dot(ray.direction, outward_normal) < 0.0;
	rec.normal = normalize(rec.front_face ? outward_normal : -outward_normal);

	return true;
}

vec3 ray_color(Ray ray, int spheres_count, Sphere spheres[2])
{
	vec3 accumulated_color = vec3(1.0); // Start with white light
	vec3 final_color = vec3(0.0);       // Accumulated final color
	int max_bounces = 5;

	for (int bounce = 0; bounce < max_bounces; bounce++)
	{
		HitRecord closest_rec;
		float closest_t = MAX_DIST; // A large value to represent infinity
		bool hit_anything = false;

		// Find the closest sphere hit
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

		if (hit_anything)
		{
			// Calculate scattered ray
			vec3 direction = closest_rec.normal + random_unit_vector();
			Ray scattered;
			scattered.origin = closest_rec.point;
			scattered.direction = direction;

			// Accumulate color
			accumulated_color *= 0.5; // Attenuation factor
			ray = scattered;         // Update ray for the next bounce
		}
		else
		{
			// Background gradient
			vec3 unit_direction = normalize(ray.direction);
			float a = 0.5 * (unit_direction.y + 1.0); // To bring in range [0.0, 1.0]
			final_color += accumulated_color * ((1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0));
			break;
		}
	}

	return final_color;
}

void main()
{
	vec4 pixel = vec4(0.075, 0.133, 0.173, 1.0);		//random pixel colors; redundant
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	seed = uint(pixel_coords.x ^ pixel_coords.y ^ uint(gl_GlobalInvocationID.x * 1973 + gl_GlobalInvocationID.y * 9277));
	
	ivec2 dims = imageSize(screen);

	float fov = 90.0;
	vec3 cam_o = vec3(0.0, 0.0, -tan(fov / 2.0));

	int samples_per_pixel = 10;
	vec3 accumulated_color = vec3(0.0);

	for (int i=0; i<samples_per_pixel; i++)
	{
		float x = -(float((pixel_coords.x + random_float()) * 2 - dims.x) / dims.x);	//transforms to [-1,1]
        float y = -(float((pixel_coords.y + random_float()) * 2 - dims.y) / dims.y);	//transforms to [-1,1]

		vec3 ray_o = vec3(x, y, 0.0);
		vec3 ray_d = normalize(cam_o - ray_o);

		Ray ray;
		ray.origin=ray_o;
		ray.direction=ray_d;

		Sphere sphere;
		sphere.position=vec3(0.0, 0.0, -6.0);
		sphere.radius=1;

		Sphere ground;
		ground.position=vec3(0.0, -101, -6.0);
		ground.radius=100.0;

		Sphere spheres[2];
		spheres[0]=sphere;
		spheres[1]=ground;

		accumulated_color += ray_color(ray, 2, spheres);
	}

	pixel = vec4(linear_to_gamma(accumulated_color / float(samples_per_pixel)), 1.0);
	imageStore(screen, pixel_coords, pixel);
}