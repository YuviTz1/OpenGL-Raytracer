#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen;

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

vec3 ray_at(float t, Ray ray) 
{
	return ray.origin + t*ray.direction;
}

float hit_sphere(Ray ray, Sphere sphere)
{
	vec3 oc = ray.origin - sphere.position;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float discriminant = b * b - 4.0 * a * c;
	
	if(discriminant < 0.0)
	{
		return -1.0;
	}
	else
	{
		return (-b - sqrt(discriminant)) / (2.0 * a);
	}
}

vec3 ray_color(Ray ray, Sphere sphere)
{
	float hit_sphere_t = hit_sphere(ray, sphere);
	if(hit_sphere_t > 0.0)
	{
		vec3 N = normalize(ray_at(hit_sphere_t, ray) - sphere.position);
		return 0.5 * vec3(N.x + 1.0, N.y + 1.0, N.z + 1.0);
	}
	
	// background gradient
	vec3 unit_direction = normalize(ray.direction);
	float a = 0.5*(unit_direction.y + 1.0);			// to bring in range [0.0, 1.0]
	return (1.0-a)*vec3(1.0, 1.0, 1.0) + a*vec3(0.5, 0.7, 1.0);
}

void main()
{
	vec4 pixel = vec4(0.075, 0.133, 0.173, 1.0);
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	
	ivec2 dims = imageSize(screen);
	float x = -(float(pixel_coords.x * 2 - dims.x) / dims.x); // transforms to [-1.0, 1.0]
	float y = -(float(pixel_coords.y * 2 - dims.y) / dims.y); // transforms to [-1.0, 1.0]

	//get ray direction and origin
	float fov = 90.0;
	vec3 cam_o = vec3(0.0, 0.0, -tan(fov / 2.0));
	vec3 ray_o = vec3(x, y, 0.0);
	vec3 ray_d = normalize(cam_o - ray_o);

	Ray ray;
	ray.origin=ray_o;
	ray.direction=ray_d;

	Sphere sphere;
	sphere.position=vec3(0.0, 0.0, -4.0);
	sphere.radius=0.5;

	pixel=vec4(ray_color(ray, sphere),1.0);

	imageStore(screen, pixel_coords, pixel);
}