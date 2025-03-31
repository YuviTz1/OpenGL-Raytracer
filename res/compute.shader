#version 460 core
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D screen;

struct sphere
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

bool hit_sphere(sphere s, Ray ray)
{
	vec3 oc = ray.origin - s.position;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - s.radius * s.radius;
	float discriminant = b * b - 4.0 * a * c;
	return (discriminant > 0.0);
}

vec3 ray_color(Ray ray)
{

	if(hit_sphere(sphere(vec3(0.0, 0.0, -5.0), 0.5), ray))
	{
		return vec3(1.0, 0.0, 0.0);
	}
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
	vec3 ray_d = normalize(ray_o - cam_o);

	Ray ray;
	ray.origin=ray_o;
	ray.direction=ray_d;

	pixel=vec4(ray_color(ray),1.0);

	imageStore(screen, pixel_coords, pixel);
}