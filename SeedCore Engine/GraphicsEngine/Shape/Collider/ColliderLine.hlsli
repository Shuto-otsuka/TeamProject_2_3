#ifndef __COLLIDER_LINE_HLSL__
#define __COLLIDER_LINE_HLSL__

#include "../../Shader/Scene.hlsli"

#define COLLIDER_SHAPE_BOX 0
#define COLLIDER_SHAPE_SPHERE 1
#define COLLIDER_SHAPE_CAPSULE 2
#define COLLIDER_SHAPE_CYLINDER 3
#define COLLIDER_SHAPE_RECT 4
#define COLLIDER_SHAPE_CIRCLE 5
#define COLLIDER_SHAPE_CONE 6

#define COLLIDER_TWO_PI 6.28318530717959
#define COLLIDER_PI 3.14159265358979

/// [EN] Segments of every full circle (rings, great circles, sphere silhouette).
#define COLLIDER_RING_SEGMENTS 32

/// [EN] Segments of every half circle (capsule cap arcs and cap silhouettes).
#define COLLIDER_ARC_SEGMENTS 16

/// [EN] Fixed vertical side lines of a capsule, cylinder or cone: front, back, left and right.
#define COLLIDER_SIDE_LINE_COUNT 4

/// [EN] On-screen line widths in pixels; the silhouette is drawn thicker so the outline reads first.
#define COLLIDER_LINE_WIDTH 3.0
#define COLLIDER_SILHOUETTE_LINE_WIDTH 4.5

/// [EN] Lines handled by one mesh-shader group. Each line becomes a quad of 4 vertices and 2 triangles, so 64 lines fill the 256-vertex mesh-shader output limit. Must match ColliderRenderer::threadsPerGroup_.
#define COLLIDER_LINES_PER_GROUP 64

/// [EN] Line count of each shape. The capsule is the densest and must match ColliderRenderer::maxLinesPerInstance_.
#define COLLIDER_BOX_LINE_COUNT 12
#define COLLIDER_SPHERE_LINE_COUNT (4 * COLLIDER_RING_SEGMENTS)
#define COLLIDER_CAPSULE_LINE_COUNT (2 * COLLIDER_RING_SEGMENTS + COLLIDER_SIDE_LINE_COUNT + 6 * COLLIDER_ARC_SEGMENTS + 2)
#define COLLIDER_CYLINDER_LINE_COUNT (2 * COLLIDER_RING_SEGMENTS + COLLIDER_SIDE_LINE_COUNT + 2)
#define COLLIDER_RECT_LINE_COUNT 4
#define COLLIDER_CIRCLE_LINE_COUNT COLLIDER_RING_SEGMENTS
#define COLLIDER_CONE_LINE_COUNT (COLLIDER_RING_SEGMENTS + COLLIDER_SIDE_LINE_COUNT + 2)

struct ColliderStructuredBuffer
{
	float3 position_;
	uint shape_kind_;
	float4 rotation_;
	float3 dimensions_;
	float collider_structured_buffer_padding_0_;
	float4 color_;
};

StructuredBuffer<ColliderStructuredBuffer> GetColliderStructuredBuffer(uint index)
{
	return ResourceDescriptorHeap[index];
}

struct ColliderConstantBuffer
{
	uint instance_buffer_index_;
	uint instance_count_;
	uint groups_per_instance_;
	uint collider_constant_buffer_padding_0_;
};

ConstantBuffer<ColliderConstantBuffer> GetColliderConstantBuffer()
{
	return ResourceDescriptorHeap[constant_indices.collider_index_];
}

struct ColliderLineMSOutput
{
	float4 position : SV_Position;
	float4 color : COLOR0;
};

/**
* [EN]
* Number of lines a shape is drawn with.
*/
uint GetColliderLineCount(uint shape_kind)
{
	if (shape_kind == COLLIDER_SHAPE_BOX)
	{
		return COLLIDER_BOX_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_SPHERE)
	{
		return COLLIDER_SPHERE_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_CAPSULE)
	{
		return COLLIDER_CAPSULE_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_CYLINDER)
	{
		return COLLIDER_CYLINDER_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_RECT)
	{
		return COLLIDER_RECT_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_CIRCLE)
	{
		return COLLIDER_CIRCLE_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_CONE)
	{
		return COLLIDER_CONE_LINE_COUNT;
	}
	return 0;
}

/**
* [EN]
* Rotates v by the unit quaternion q (xyz = vector part, w = scalar part).
*/
float3 RotateByQuaternion(float4 q, float3 v)
{
	float3 t = 2.0 * cross(q.xyz, v);
	return v + q.w * t + cross(q.xyz, t);
}

/**
* [EN]
* Point on a horizontal circle of radius r at height y; angle 0 lies on +X and the circle turns toward +Z.
*/
float3 RingPoint(float r, float y, float angle)
{
	return float3(r * cos(angle), y, r * sin(angle));
}

/**
* [EN]
* One segment of a horizontal ring of radius r at height y.
*/
void GetRingLine(float r, float y, uint segment, out float3 a, out float3 b)
{
	float angle0 = (float(segment) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	float angle1 = (float(segment + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	a = RingPoint(r, y, angle0);
	b = RingPoint(r, y, angle1);
}

/**
* [EN]
* Silhouette of a sphere seen from a point camera: the circle where the
* view cone touches the sphere. It lies in the plane facing the camera,
* pulled toward the camera by r^2/d from the center, with radius
* r * sqrt(d^2 - r^2) / d, where d is the camera distance. Returns false
* when the camera is inside the sphere, which has no silhouette.
*/
bool GetSphereSilhouette(float3 center, float r, float3 local_camera, out float3 circle_center, out float circle_radius, out float3 view_direction)
{
	float3 to_camera = local_camera - center;
	float distance_to_camera = length(to_camera);

	circle_center = center;
	circle_radius = 0.0;
	view_direction = float3(0.0, 0.0, 1.0);

	if (distance_to_camera <= r)
	{
		return false;
	}

	view_direction = to_camera / distance_to_camera;
	circle_center = center + view_direction * (r * r / distance_to_camera);
	circle_radius = r * sqrt(distance_to_camera * distance_to_camera - r * r) / distance_to_camera;
	return true;
}

/**
* [EN]
* Silhouette half circle of one capsule cap (sign +1 = top, -1 = bottom).
* The half is bounded by the two points on the sides of the capsule axis
* (along u, perpendicular to both the axis and the view direction) and
* bulges away from the other cap (along v). Returns false when the camera
* is inside the cap sphere.
*/
bool GetCapSilhouette(float r, float h, float sign, float3 local_camera, out float3 circle_center, out float circle_radius, out float3 u, out float3 v)
{
	float3 view_direction;
	bool valid = GetSphereSilhouette(float3(0.0, sign * h, 0.0), r, local_camera, circle_center, circle_radius, view_direction);

	/// [EN] Looking straight along the axis, every direction across it is a side; X is used.
	u = cross(float3(0.0, 1.0, 0.0), view_direction);
	u = (dot(u, u) > 1.0e-8) ? normalize(u) : float3(1.0, 0.0, 0.0);

	v = cross(view_direction, u);
	if (dot(v, float3(0.0, 1.0, 0.0)) * sign < 0.0)
	{
		v = -v;
	}

	return valid;
}

void GetBoxLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	uint2 edges[12] =
	{
		uint2(0, 1), uint2(0, 2), uint2(0, 4), uint2(1, 3),
		uint2(1, 5), uint2(2, 3), uint2(2, 6), uint2(3, 7),
		uint2(4, 5), uint2(4, 6), uint2(5, 7), uint2(6, 7)
	};

	uint2 edge = edges[line_index];
	a = float3((edge.x & 1) ? dimensions.x : -dimensions.x, (edge.x & 2) ? dimensions.y : -dimensions.y, (edge.x & 4) ? dimensions.z : -dimensions.z);
	b = float3((edge.y & 1) ? dimensions.x : -dimensions.x, (edge.y & 2) ? dimensions.y : -dimensions.y, (edge.y & 4) ? dimensions.z : -dimensions.z);
}

/**
* [EN]
* Three great circles on the XY, YZ and XZ planes plus the silhouette
* circle facing the camera. dimensions.x is the radius.
*/
void GetSphereLine(float3 dimensions, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	float r = dimensions.x;
	uint circle_index = line_index / COLLIDER_RING_SEGMENTS;
	uint segment = line_index % COLLIDER_RING_SEGMENTS;
	float angle0 = (float(segment) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	float angle1 = (float(segment + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	float2 point0 = float2(cos(angle0), sin(angle0)) * r;
	float2 point1 = float2(cos(angle1), sin(angle1)) * r;

	silhouette = false;

	if (circle_index == 0)
	{
		a = float3(point0.x, point0.y, 0.0);
		b = float3(point1.x, point1.y, 0.0);
		return;
	}

	if (circle_index == 1)
	{
		a = float3(0.0, point0.x, point0.y);
		b = float3(0.0, point1.x, point1.y);
		return;
	}

	if (circle_index == 2)
	{
		a = float3(point0.x, 0.0, point0.y);
		b = float3(point1.x, 0.0, point1.y);
		return;
	}

	silhouette = true;

	float3 circle_center;
	float circle_radius;
	float3 view_direction;
	GetSphereSilhouette(float3(0.0, 0.0, 0.0), r, local_camera, circle_center, circle_radius, view_direction);

	/// [EN] Any pair of axes perpendicular to the view direction spans the silhouette plane.
	float3 u = normalize(cross(abs(view_direction.y) > 0.99 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0), view_direction));
	float3 v = cross(view_direction, u);

	a = circle_center + (u * cos(angle0) + v * sin(angle0)) * circle_radius;
	b = circle_center + (u * cos(angle1) + v * sin(angle1)) * circle_radius;
}

/**
* [EN]
* Capsule along Y: two rings where the caps meet the body, four side
* lines, a half circle per cap on the XY and ZY planes, and the
* silhouette - a half circle per cap facing the camera joined by two side
* lines. dimensions = (radius, halfHeightOfCylinder, unused).
*/
void GetCapsuleLine(float3 dimensions, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	float r = dimensions.x;
	float h = dimensions.y;
	uint index = line_index;

	silhouette = false;

	/// [EN] Rings at the bottom and top of the body.
	if (index < 2 * COLLIDER_RING_SEGMENTS)
	{
		GetRingLine(r, (index < COLLIDER_RING_SEGMENTS) ? -h : h, index % COLLIDER_RING_SEGMENTS, a, b);
		return;
	}
	index -= 2 * COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed side lines.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = RingPoint(r, -h, angle);
		b = RingPoint(r, h, angle);
		return;
	}
	index -= COLLIDER_SIDE_LINE_COUNT;

	/// [EN] Cap arcs: top on XY, top on ZY, bottom on XY, bottom on ZY.
	if (index < 4 * COLLIDER_ARC_SEGMENTS)
	{
		uint arc_index = index / COLLIDER_ARC_SEGMENTS;
		uint segment = index % COLLIDER_ARC_SEGMENTS;
		float sign = (arc_index < 2) ? 1.0 : -1.0;
		float3 across = (arc_index % 2 == 0) ? float3(1.0, 0.0, 0.0) : float3(0.0, 0.0, 1.0);
		float3 center = float3(0.0, sign * h, 0.0);
		float angle0 = (float(segment) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		float angle1 = (float(segment + 1) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		a = center + (across * cos(angle0) + float3(0.0, sign, 0.0) * sin(angle0)) * r;
		b = center + (across * cos(angle1) + float3(0.0, sign, 0.0) * sin(angle1)) * r;
		return;
	}
	index -= 4 * COLLIDER_ARC_SEGMENTS;

	silhouette = true;

	/// [EN] Silhouette half circle of each cap.
	if (index < 2 * COLLIDER_ARC_SEGMENTS)
	{
		float sign = (index < COLLIDER_ARC_SEGMENTS) ? 1.0 : -1.0;
		uint segment = index % COLLIDER_ARC_SEGMENTS;

		float3 circle_center;
		float circle_radius;
		float3 u;
		float3 v;
		GetCapSilhouette(r, h, sign, local_camera, circle_center, circle_radius, u, v);

		float angle0 = (float(segment) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		float angle1 = (float(segment + 1) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		a = circle_center + (u * cos(angle0) + v * sin(angle0)) * circle_radius;
		b = circle_center + (u * cos(angle1) + v * sin(angle1)) * circle_radius;
		return;
	}
	index -= 2 * COLLIDER_ARC_SEGMENTS;

	/// [EN] Silhouette side lines join the ends of the two cap half circles, so the outline is one closed loop.
	float side = (index == 0) ? 1.0 : -1.0;

	float3 bottom_center;
	float bottom_radius;
	float3 bottom_u;
	float3 bottom_v;
	GetCapSilhouette(r, h, -1.0, local_camera, bottom_center, bottom_radius, bottom_u, bottom_v);

	float3 top_center;
	float top_radius;
	float3 top_u;
	float3 top_v;
	GetCapSilhouette(r, h, 1.0, local_camera, top_center, top_radius, top_u, top_v);

	a = bottom_center + bottom_u * (side * bottom_radius);
	b = top_center + top_u * (side * top_radius);
}

/**
* [EN]
* Cylinder along Y: top and bottom rings, four side lines, and the two
* silhouette side lines. The silhouette lines stand where the lines from
* the camera touch the rings, found from the camera's position projected
* onto the XZ plane. dimensions = (radius, halfHeight, unused).
*/
void GetCylinderLine(float3 dimensions, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	float r = dimensions.x;
	float h = dimensions.y;
	uint index = line_index;

	silhouette = false;

	/// [EN] Rings at the bottom and top.
	if (index < 2 * COLLIDER_RING_SEGMENTS)
	{
		GetRingLine(r, (index < COLLIDER_RING_SEGMENTS) ? -h : h, index % COLLIDER_RING_SEGMENTS, a, b);
		return;
	}
	index -= 2 * COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed side lines.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = RingPoint(r, -h, angle);
		b = RingPoint(r, h, angle);
		return;
	}
	index -= COLLIDER_SIDE_LINE_COUNT;

	silhouette = true;

	/// [EN] A camera inside the infinite cylinder sees no side silhouette, so the line collapses to a point.
	float2 camera_plane = local_camera.xz;
	float camera_distance = length(camera_plane);
	if (camera_distance <= r)
	{
		a = float3(0.0, 0.0, 0.0);
		b = a;
		return;
	}

	/// [EN] The tangent point is acos(r / d) away from the direction toward the camera.
	float side = (index == 0) ? 1.0 : -1.0;
	float angle = atan2(camera_plane.y, camera_plane.x) + side * acos(r / camera_distance);
	a = RingPoint(r, -h, angle);
	b = RingPoint(r, h, angle);
}

/**
* [EN]
* Cone along Y with its base ring at -halfHeight and its apex at
* +halfHeight: the base ring, four lines to the apex, and the two
* silhouette lines to the apex. A silhouette line touches the base where
* the line through the apex and the camera meets the base plane and is
* tangent to the ring from there. dimensions = (radius, halfHeight, unused).
*/
void GetConeLine(float3 dimensions, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	float r = dimensions.x;
	float h = dimensions.y;
	float3 apex = float3(0.0, h, 0.0);
	uint index = line_index;

	silhouette = false;

	/// [EN] Base ring.
	if (index < COLLIDER_RING_SEGMENTS)
	{
		GetRingLine(r, -h, index, a, b);
		return;
	}
	index -= COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed lines from the base to the apex.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = RingPoint(r, -h, angle);
		b = apex;
		return;
	}
	index -= COLLIDER_SIDE_LINE_COUNT;

	silhouette = true;

	float side = (index == 0) ? 1.0 : -1.0;
	float3 apex_to_camera = local_camera - apex;
	float base_angle;
	float tangent_offset;

	if (abs(apex_to_camera.y) < 1.0e-5)
	{
		/// [EN] A camera level with the apex sees the cone edge-on; the tangent points are a quarter turn from the camera direction.
		base_angle = atan2(apex_to_camera.z, apex_to_camera.x);
		tangent_offset = COLLIDER_PI * 0.5;
	}
	else
	{
		/// [EN] Where the line through the apex and the camera meets the base plane.
		float t = (-2.0 * h) / apex_to_camera.y;
		float2 base_point = apex_to_camera.xz * t;
		float base_distance = length(base_point);

		/// [EN] A camera looking into the cone from above the apex or below the base sees no side silhouette.
		if (base_distance <= r)
		{
			a = apex;
			b = apex;
			return;
		}

		base_angle = atan2(base_point.y, base_point.x);
		tangent_offset = acos(r / base_distance);
	}

	a = RingPoint(r, -h, base_angle + side * tangent_offset);
	b = apex;
}

void GetRectLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	float hx = dimensions.x;
	float hy = dimensions.y;

	float3 corners[4] =
	{
		float3(-hx, -hy, 0.0), float3(hx, -hy, 0.0),
		float3(hx, hy, 0.0), float3(-hx, hy, 0.0)
	};

	a = corners[line_index];
	b = corners[(line_index + 1) % 4];
}

void GetCircleLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	float r = dimensions.x;
	float angle0 = (float(line_index) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	float angle1 = (float(line_index + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	a = float3(r * cos(angle0), r * sin(angle0), 0.0);
	b = float3(r * cos(angle1), r * sin(angle1), 0.0);
}

/**
* [EN]
* Local-space endpoints of one line of a shape, and whether that line
* belongs to the camera-facing silhouette. local_camera is the camera
* position in the shape's local space.
*/
void GetColliderLine(uint shape_kind, float3 dimensions, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	a = float3(0.0, 0.0, 0.0);
	b = float3(0.0, 0.0, 0.0);
	silhouette = false;

	if (shape_kind == COLLIDER_SHAPE_BOX)
	{
		GetBoxLine(dimensions, line_index, a, b);
	}
	else if (shape_kind == COLLIDER_SHAPE_SPHERE)
	{
		GetSphereLine(dimensions, local_camera, line_index, a, b, silhouette);
	}
	else if (shape_kind == COLLIDER_SHAPE_CAPSULE)
	{
		GetCapsuleLine(dimensions, local_camera, line_index, a, b, silhouette);
	}
	else if (shape_kind == COLLIDER_SHAPE_CYLINDER)
	{
		GetCylinderLine(dimensions, local_camera, line_index, a, b, silhouette);
	}
	else if (shape_kind == COLLIDER_SHAPE_RECT)
	{
		GetRectLine(dimensions, line_index, a, b);
	}
	else if (shape_kind == COLLIDER_SHAPE_CIRCLE)
	{
		GetCircleLine(dimensions, line_index, a, b);
	}
	else if (shape_kind == COLLIDER_SHAPE_CONE)
	{
		GetConeLine(dimensions, local_camera, line_index, a, b, silhouette);
	}
}

/**
* [EN]
* Silhouette lines are drawn halfway to white so the outline stands out from the rest of the shape.
*/
float4 GetColliderLineColor(float4 color, bool silhouette)
{
	return silhouette ? float4(lerp(color.rgb, float3(1.0, 1.0, 1.0), 0.5), color.a) : color;
}

/**
* [EN]
* One corner of the screen-space quad that draws a clip-space line
* width pixels thick. Corners 0 and 1 sit on the a end, 2 and 3 on the b
* end, on opposite sides of the line; each end is also pushed out by
* half the width so neighbouring segments of a curve overlap at their
* joints.
*/
float4 ExpandColliderLine(float4 clip_a, float4 clip_b, float2 display_size, float width, uint corner)
{
	/// [EN] An end behind the camera has no screen position, so the line is cut where it crosses just in front of the eye.
	const float near_w = 1.0e-3;
	if (clip_a.w < near_w && clip_b.w < near_w)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}
	if (clip_a.w < near_w)
	{
		clip_a = lerp(clip_a, clip_b, (near_w - clip_a.w) / (clip_b.w - clip_a.w));
	}
	if (clip_b.w < near_w)
	{
		clip_b = lerp(clip_b, clip_a, (near_w - clip_b.w) / (clip_a.w - clip_b.w));
	}

	/// [EN] The direction is taken in pixels so the width stays even however the viewport is stretched.
	float2 pixel_a = (clip_a.xy / clip_a.w) * display_size;
	float2 pixel_b = (clip_b.xy / clip_b.w) * display_size;
	float2 pixel_direction = pixel_b - pixel_a;
	float pixel_length = length(pixel_direction);
	if (pixel_length < 1.0e-6)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}
	pixel_direction /= pixel_length;
	float2 pixel_normal = float2(-pixel_direction.y, pixel_direction.x);

	/// [EN] Normalized device coordinates span 2 across display_size pixels, so half the width in pixels is width / display_size in them.
	float2 to_ndc = width / display_size;
	float2 side_offset = pixel_normal * to_ndc * (((corner & 1) != 0) ? 1.0 : -1.0);
	float2 end_offset = pixel_direction * to_ndc * ((corner < 2) ? -1.0 : 1.0);

	/// [EN] Offsets are scaled by w so they survive the perspective divide unchanged.
	float4 clip_position = (corner < 2) ? clip_a : clip_b;
	clip_position.xy += (side_offset + end_offset) * clip_position.w;
	return clip_position;
}

#endif // __COLLIDER_LINE_HLSL__
