#version 120
#define SEGMENTS_TOTAL 3
//#define DEBUG
//#define DEBUG_DRAW_SKEL
#define IMG_WIDTH 620
#define IMG_HEIGHT 349

struct Segment
{
  vec2 p;
  vec2 v;
  float n;
};

uniform Segment segmentsSrc[SEGMENTS_TOTAL];
uniform Segment segmentsDst[SEGMENTS_TOTAL];
uniform Segment segmentsMid[SEGMENTS_TOTAL];
uniform float alpha;
uniform sampler2D texSrc;
uniform sampler2D texDst;
uniform mat3x2 transSrc;
uniform mat3x2 transDst;

varying vec2 texCoord;

float calcPointDistanceToSegment(Segment seg, vec2 point)
{
	float u = clamp(dot(point-seg.p, seg.v), 0, seg.n);
	vec2 projectedPoint = seg.p + seg.v*u;
	float A = distance(projectedPoint, point);
	return A;
}

vec2 warpPointFromSegToSeg(Segment segSrc, Segment segDst, vec2 point)
{
	float a = dot(point - segSrc.p, segSrc.v);
	float b = dot(point - segSrc.p, vec2(-segSrc.v.y, segSrc.v.x));
	float scale = segDst.n/segSrc.n;
	return segDst.p + scale*a*segDst.v + scale*b*vec2(-segDst.v.y, segDst.v.x);
} 
/*
vec2 warpPointFromSegToSeg(Segment segSrc, Segment segDst, vec2 point)
{
	float a = dot(point - segSrc.p, segSrc.v);
	float b = dot(point - segSrc.p, vec2(-segSrc.v.y, segSrc.v.x));
	
	return segDst.p + a*segDst.v + b*vec2(-segDst.v.y, segDst.v.x);
}            
*/
/*
vec2 warpPointFromSegToSeg(Segment segSrc, Segment segDst, vec2 point)
{
	float u = clamp(dot(point-segSrc.p, segSrc.v), 0, segSrc.n);
	float uNorm = u/segSrc.n;
	vec2 projectedPoint = segSrc.p + segSrc.v*u;	
	vec2 offset = point - projectedPoint;	
	return segDst.p + uNorm*segDst.v + offset;
}            
*/
void drawSkel(Segment segments[SEGMENTS_TOTAL], vec4 color)
{
	for(int i=0; i<SEGMENTS_TOTAL; ++i)
	{
		float dist = calcPointDistanceToSegment(segments[i], texCoord);
		if(dist < 2)
		{
			gl_FragColor = color;		
			return;
		}
	}
}

vec3 colors[] = vec3[3](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));

void main() 
{
	vec3 color = vec3(0);
	float weightSum = 0;
	vec2 warpedTexCoordSrc = vec2(0,0);
	vec2 warpedTexCoordDst = vec2(0,0);
	for(int i=0; i<SEGMENTS_TOTAL; ++i)
	{
		float dist = calcPointDistanceToSegment(segmentsMid[i], texCoord);
		//float weight = 1/(dist+0.00001);
		float weight = exp(-dist/10);
		//float weight = 1;
		weight = max(weight, 0.001);
		weightSum += weight;
		warpedTexCoordSrc += weight * warpPointFromSegToSeg(segmentsMid[i], segmentsSrc[i], texCoord);
		warpedTexCoordDst += weight * warpPointFromSegToSeg(segmentsMid[i], segmentsDst[i], texCoord);
		color += weight * colors[i];
	}
	warpedTexCoordSrc /= weightSum;
	warpedTexCoordDst /= weightSum;
#ifdef DEBUG
	gl_FragColor = vec4(color/weightSum,1);                
#else

	gl_FragColor = mix(
		texture2D(texSrc, (transSrc * vec3(warpedTexCoordSrc,1)).xy / vec2(IMG_WIDTH, IMG_HEIGHT)),
		texture2D(texDst, (transDst * vec3(warpedTexCoordDst,1)).xy / vec2(IMG_WIDTH, IMG_HEIGHT)),
		alpha);

		//gl_FragColor = texture2D(texSrc, (transSrc * vec3(warpedTexCoordSrc,1)).xy / vec2(640,480));
#endif		
#ifdef DEBUG_DRAW_SKEL
	drawSkel(segmentsSrc, vec4(1,0,0,1));
	drawSkel(segmentsMid, vec4(1,1,0,1));
	drawSkel(segmentsDst, vec4(0,1,0,1));
#endif
}
