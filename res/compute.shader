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

vec3 ray_color(Ray ray)
{
	vec3 unit_direction = normalize(ray.direction);
	float a = 0.5*(unit_direction.y + 1.0);
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

	// vec3 sphere_c = vec3(0.0, 0.0, -5.0);
	// float sphere_r = 1.0;

	// vec3 o_c = ray_o - sphere_c;
	// float b = dot(ray_d, o_c);
	// float c = dot(o_c, o_c) - sphere_r * sphere_r;
	// float intersectionState = b * b - c;
	// vec3 intersection = ray_o + ray_d * (-b + sqrt(b * b - c));

	// if (intersectionState >= 0.0)
	// {
	// 	pixel = vec4((normalize(intersection - sphere_c) + 1.0) / 2.0, 1.0);
	// }

	pixel=vec4(ray_color(ray),1.0);

	imageStore(screen, pixel_coords, pixel);
}