#define IMAGE_SIZE 256
#define CANVAS_SIZE 10.0f
#define GRAVITY 0.0005f
#define THRESHOLD_VELOCITY 0.0001f
#define BOUNCE_FACTOR 0.33f

__kernel void animate(__global float4 *particles_position_vbo, __global float4 *particles_velocity_vbo, __write_only image2d_t floorIm, __write_only image2d_t wallIm, __global float4 *particles_color_vbo, __global uint *particles_wait_time, uint frame)
{
    int gid = get_global_id(0);     

	// is rounded to zero?
	if(isnan(particles_velocity_vbo[gid].x))
		return;

	if(particles_wait_time[gid]>frame)
		return;
		
	// Advance 
    particles_position_vbo[gid].xyz += particles_velocity_vbo[gid].xyz;
    
    // Check collision with surface
	if(particles_position_vbo[gid].y <= 0)
	{		
		// Reflect
		particles_velocity_vbo[gid].y *= -1;
		particles_position_vbo[gid].y *= -1;
		
		// Decrease bounce
		particles_velocity_vbo[gid].xyz *= BOUNCE_FACTOR;			
		
		// Round to zero?		
		if(length(particles_velocity_vbo[gid]) < THRESHOLD_VELOCITY)
		{
			particles_velocity_vbo[gid].x = NAN;
			particles_position_vbo[gid].y = 0.001f;
		}
		
		// Find image coordinates
		float2 pos = particles_position_vbo[gid].xz;
		float2 relPos = (float2)((pos.x + CANVAS_SIZE/2.0f)/CANVAS_SIZE, pos.y/CANVAS_SIZE);
		int2 floorCoord = convert_int2(relPos*IMAGE_SIZE);
		
		// If inside image, color it			
		if(	floorCoord.x >= 0 && floorCoord.x < IMAGE_SIZE &&
			floorCoord.y >= 0 && floorCoord.y < IMAGE_SIZE)
		{
			float4 colorf = particles_color_vbo[gid];
			write_imagef(floorIm, floorCoord,colorf);					
		}
	}	
	if(particles_position_vbo[gid].z <= 0)
	{		
		// Reflect
		particles_velocity_vbo[gid].z *= -1;
		particles_position_vbo[gid].z *= -1;
		
		// Decrease bounce
		particles_velocity_vbo[gid].xyz *= BOUNCE_FACTOR;			
		
		// Round to zero?		
		if(length(particles_velocity_vbo[gid]) < THRESHOLD_VELOCITY)
		{
			particles_velocity_vbo[gid].x = NAN;
			particles_position_vbo[gid].z = 0.001f;
		}

		
		// Find image coordinates
		float2 pos = particles_position_vbo[gid].xy;
		float2 relPos = (float2)((pos.x + CANVAS_SIZE/2.0f)/CANVAS_SIZE, pos.y/CANVAS_SIZE);
		int2 floorCoord = convert_int2(relPos*IMAGE_SIZE);
		
		// If inside image, color it			
		if(	floorCoord.x >= 0 && floorCoord.x < IMAGE_SIZE &&
			floorCoord.y >= 0 && floorCoord.y < IMAGE_SIZE)
		{
			float4 colorf = particles_color_vbo[gid];
			write_imagef(wallIm, floorCoord,colorf);					
		}
	
	}	
    particles_velocity_vbo[gid].y -= GRAVITY;
}
