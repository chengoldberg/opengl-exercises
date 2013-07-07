// External
typedef struct {
   float3 emission;
   float3 ambient;   
   float3 diffuse;
   float3 specular;
   float shininess;
} Material;

typedef struct {
   float4 center;
   Material mat;   
   float radius;
} Sphere;

typedef struct {
   float4 pos;
   float4 color;
} Light;

typedef struct {
   float4 pos;   
   float4 back;
   float4 right;
   float4 up;
} Camera;

typedef struct {
    float left;
    float right;
    float bottom;
    float top;
    float near;
    float far;
} Frustum;

typedef struct {
   float4 backgroundCol;
   float3 ambientLight;
   Light light;
   Sphere surface;
   Camera camera;
   Frustum projection;
} Scene;

// Internal

typedef struct {
   float4 p;
   float4 v;
} Ray;

typedef struct {
   float4 intersection;
   Sphere* surface;
   uchar isMiss;
} Hit;

typedef struct {
    float4 color;
    float depth;
} Fragment;

Hit createHit(float4 intersection, Sphere* surface, uchar isMiss)
{
    Hit h;
    h.intersection = intersection;
    h.surface = surface;
    h.isMiss = isMiss;
    return h;
}

Ray createRay(float4 p, float4 v) 
{
    Ray r;
    r.p = p;
    r.v = v;
    return r;
}

Fragment createFragment(float4 color, float depth)
{
    Fragment f;
    f.color = color;
    f.depth = depth;
    return f;
}

float3 reflect(float3 L, float3 N) 
{
    return -L + 2*N*(dot(L,N));    
} 

float3 Sphere_normalAt(Sphere* self, float4 intersection, Ray ray) 
{
    float3 v = (intersection - self->center).xyz;
    return normalize(v);
}

float Sphere_nearestIntersection(Sphere* self, Ray ray) 
{
   float4 v = self->center - ray.p;
   float qB = dot(v,ray.v);
   float delta = (qB*qB) - pow(length(ray.v),2) * (pow(length(v),2)-pow(self->radius,2));
   
   if (delta <= 0)
      return 9999.0; // No intersection
      
   float sqDelta = sqrt(delta);
   float tp = (qB + sqDelta);
   if(tp < 0)
      return 9999.0;
   float tn = (qB - sqDelta);
   
   return (tn > 0.0) ? tn : tp; // Inside or outside      
}

Hit Scene_findIntersection(Scene* self, Ray ray) 
{     
    float minDistance = 9999.0;
    Sphere* minSurface;

    minDistance = Sphere_nearestIntersection(&self->surface, ray);
    minSurface = &self->surface;
    if(minDistance ==  9999.0)
        return createHit((float4)(0), minSurface, true);
      
   float4 intersection = ray.p + minDistance*ray.v;
   
   return createHit(intersection, minSurface, false);
}

float4 Scene_calcColor(Scene* self, Hit hit, Ray ray) 
{
   if(hit.isMiss) 
      return self->backgroundCol;
   Material mat = hit.surface->mat;
   float3 normal = Sphere_normalAt(hit.surface, hit.intersection, ray).xyz;
         
   float3 res = mat.emission;
   res += self->ambientLight*mat.ambient;

    Light* lights[1];
    lights[0] = &self->light;
    
    
   float3 lightTerm;
   for(int i=0;i<1;++i) {
   
      float3 lightTerm = (float3)(0);
      
      float3 L = normalize((lights[i]->pos - hit.intersection).xyz);
      float3 V = normalize((ray.v).xyz);
      float3 R = reflect(L,normal.xyz);
      
      float da= dot(normal, L);      
      if(da < 0)
         da = 0;         
         
      float sa = dot(R, L);
      if(sa < 0)
         sa = 0;
       
      lightTerm = (da*mat.diffuse + pow(sa, mat.shininess)*mat.specular)*lights[i]->color.xyz;                           
      res += lightTerm;      
   }
   
   return (float4)(res,1.0);
}

Ray constructRayThroughPixelOrtho(int2 p, Camera* camera) 
{
   return createRay((float4)(convert_float2(p), 0.0,1.0), (float4)(0,0,-1,0));
}

Ray constructRayThroughPixelPersp(int2 p, Scene* scene) 
{
    Camera* camera = &scene->camera;    
    Frustum* frustum = &scene->projection;

    // Origin is bottm-left of near-plane rectangle
	float4 centerNear = camera->pos + camera->back*frustum->near;
    float4 p00 = centerNear + frustum->left*camera->right + frustum->bottom*camera->up;

	float2 pr = convert_float2(p)/(float2)(512.0, 512.0);
    float4 pf = p00 + pr.x*camera->right*(frustum->right - frustum->left) + pr.y*camera->up*(frustum->top - frustum->bottom);
    float4 ptr = normalize(pf - camera->pos);  
    return createRay(pf, ptr);
}

Fragment castRay(Scene* scene, int2 p) 
{
   Ray ray = constructRayThroughPixelPersp(p, scene);
   //Ray ray = constructRayThroughPixelOrtho(p, &scene->camera);
//   return (float4)(ray.v.xyz,1);
//    return ray.p;

   Hit hit = Scene_findIntersection(scene, ray);
        
   Fragment fragment;
//   fragment.depth = (dot(hit.intersection - ray.p, ray.v)-scene->projection.near)/(scene->projection.far-scene->projection.near);
//   fragment.depth = (dot(hit.intersection - ray.p, scene->camera.back)-scene->projection.near)/(scene->projection.far-scene->projection.near);

   float Z = -dot(hit.intersection - scene->camera.pos, scene->camera.back);
   float C = -(scene->projection.far+scene->projection.near)/(scene->projection.far-scene->projection.near);
   float D = -2*(scene->projection.far*scene->projection.near)/(scene->projection.far-scene->projection.near);
   fragment.depth = (Z*C+D)/(-Z);
//   fragment.depth = (Z - scene->projection.near)/(scene->projection.far - scene->projection.near);
   
   if(hit.isMiss)
       fragment.depth = -1;
   fragment.color = Scene_calcColor(scene, hit, ray);
   return fragment;
}

__kernel void render(__write_only image2d_t img, __constant Scene *scene)
{
    int gid0 = get_global_id(0);  // X    
	int gid1 = get_global_id(1);  // Y
	int2 coord = (int2)(gid0, gid1);
//	float4 color = (float4)(scene->surface.center.x,0,0,0.5);
    __private Scene scenePrivate = *scene;
	Fragment fragment = castRay(&scenePrivate, coord);
    float4 res = (float4)(fragment.color.xyz, fragment.depth);
    
    // Image origin is bottom-left
  	write_imagef(img, coord, res);					
  
	//write_imagef(img, coord, (float4)(fragment.depth,0,0,1));					
}
