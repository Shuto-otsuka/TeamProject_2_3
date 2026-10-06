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
#define COLLIDER_SHAPE_SEGMENT 7
#define COLLIDER_SHAPE_ARROW 8

#define COLLIDER_TWO_PI 6.28318530717959
#define COLLIDER_PI 3.14159265358979

/// [EN] Segments of every full circle (rings, great circles, sphere silhouette).
#define COLLIDER_RING_SEGMENTS 32

/// [EN] Segments of every half circle (capsule cap arcs and cap silhouettes).
#define COLLIDER_ARC_SEGMENTS 16

/// [EN] Fixed vertical side lines of a capsule, cylinder or cone: front, back, left and right.
#define COLLIDER_SIDE_LINE_COUNT 4

/// [EN] Arrowhead length as a fraction of the arrow's length, and its base radius as a fraction of the head length.
#define COLLIDER_ARROW_HEAD_LENGTH_RATIO 0.2
#define COLLIDER_ARROW_HEAD_WIDTH_RATIO 0.4

/// [EN] Segments of an arrowhead's base ring; fewer than a full ring, since arrows are often drawn in large numbers.
#define COLLIDER_ARROW_RING_SEGMENTS 16

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
#define COLLIDER_SEGMENT_LINE_COUNT 1
#define COLLIDER_ARROW_LINE_COUNT (1 + COLLIDER_ARROW_RING_SEGMENTS + COLLIDER_SIDE_LINE_COUNT + 2)

struct ColliderStructuredBuffer
{
	float3 position_;
	uint shape_kind_;
	float4 rotation_;
	float3 dimensions_;
	/// [EN] Arrow only: upper bound of the head's length, in the same units as dimensions_; 0 leaves the head at its fixed fraction of the arrow's length.
	float head_length_;
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
	if (shape_kind == COLLIDER_SHAPE_SEGMENT)
	{
		return COLLIDER_SEGMENT_LINE_COUNT;
	}
	if (shape_kind == COLLIDER_SHAPE_ARROW)
	{
		return COLLIDER_ARROW_LINE_COUNT;
	}
	return 0;
}

/**
* [EN]
* One of the 12 edges of a box centered on the origin. dimensions holds
* the half extents. Each corner is numbered 0-7 by its sign bits: bit 0
* set means +X, bit 1 +Y and bit 2 +Z, otherwise the negative side.
*/
void GetBoxLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	/// [EN] Corner pairs that differ in exactly one bit, which are the corners joined by an edge.
	uint2 edges[12] =
	{
		uint2(0, 1), uint2(0, 2), uint2(0, 4), uint2(1, 3),
		uint2(1, 5), uint2(2, 3), uint2(2, 6), uint2(3, 7),
		uint2(4, 5), uint2(4, 6), uint2(5, 7), uint2(6, 7)
	};

	/// [EN] Each corner's sign bits pick + or - of the half extent on each axis.
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

	/// [EN] The silhouette is the circle where the view cone from the camera touches the sphere. It lies in the plane facing the camera, pulled toward the camera by r^2/d from the center, with radius r * sqrt(d^2 - r^2) / d, where d is the camera distance.
	float camera_distance = length(local_camera);

	/// [EN] A camera inside the sphere sees no silhouette, so the line collapses to a point.
	if (camera_distance <= r)
	{
		a = float3(0.0, 0.0, 0.0);
		b = a;
		return;
	}

	float3 view_direction = local_camera / camera_distance;
	float3 circle_center = view_direction * (r * r / camera_distance);
	float circle_radius = r * sqrt(camera_distance * camera_distance - r * r) / camera_distance;

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

	/// [EN] Rings at the bottom and top of the body; angle 0 lies on +X and the ring turns toward +Z.
	if (index < 2 * COLLIDER_RING_SEGMENTS)
	{
		float y = (index < COLLIDER_RING_SEGMENTS) ? -h : h;
		uint segment = index % COLLIDER_RING_SEGMENTS;
		float angle0 = (float(segment) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		float angle1 = (float(segment + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle0), y, r * sin(angle0));
		b = float3(r * cos(angle1), y, r * sin(angle1));
		return;
	}
	index -= 2 * COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed side lines.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle), -h, r * sin(angle));
		b = float3(r * cos(angle), h, r * sin(angle));
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

	/// [EN] Silhouette half circle of each cap (top first, then bottom). Each cap is a sphere whose silhouette is the circle where the view cone from the camera touches it: in the plane facing the camera, pulled toward the camera by r^2/d from the cap center, with radius r * sqrt(d^2 - r^2) / d.
	if (index < 2 * COLLIDER_ARC_SEGMENTS)
	{
		float sign = (index < COLLIDER_ARC_SEGMENTS) ? 1.0 : -1.0;
		uint segment = index % COLLIDER_ARC_SEGMENTS;
		float3 cap_center = float3(0.0, sign * h, 0.0);
		float3 to_camera = local_camera - cap_center;
		float camera_distance = length(to_camera);

		/// [EN] A camera inside the cap sphere sees no silhouette, so the line collapses to a point.
		if (camera_distance <= r)
		{
			a = cap_center;
			b = a;
			return;
		}

		float3 view_direction = to_camera / camera_distance;
		float3 circle_center = cap_center + view_direction * (r * r / camera_distance);
		float circle_radius = r * sqrt(camera_distance * camera_distance - r * r) / camera_distance;

		/// [EN] The half circle runs between the two points beside the capsule axis (along u, perpendicular to both the axis and the view direction); looking straight along the axis, every direction across it is a side and X is used.
		float3 u = cross(float3(0.0, 1.0, 0.0), view_direction);
		u = (dot(u, u) > 1.0e-8) ? normalize(u) : float3(1.0, 0.0, 0.0);

		/// [EN] It bulges away from the other cap (along v).
		float3 v = cross(view_direction, u);
		if (dot(v, float3(0.0, 1.0, 0.0)) * sign < 0.0)
		{
			v = -v;
		}

		float angle0 = (float(segment) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		float angle1 = (float(segment + 1) / float(COLLIDER_ARC_SEGMENTS)) * COLLIDER_PI;
		a = circle_center + (u * cos(angle0) + v * sin(angle0)) * circle_radius;
		b = circle_center + (u * cos(angle1) + v * sin(angle1)) * circle_radius;
		return;
	}
	index -= 2 * COLLIDER_ARC_SEGMENTS;

	/// [EN] Silhouette side lines join the ends of the two cap half circles, so the outline is one closed loop. The ends are found the same way as for the half circles above, for the bottom cap and then the top cap.
	float side = (index == 0) ? 1.0 : -1.0;
	float3 ends[2];
	for (uint cap_index = 0; cap_index < 2; cap_index++)
	{
		float3 cap_center = float3(0.0, (cap_index == 0) ? -h : h, 0.0);
		float3 to_camera = local_camera - cap_center;
		float camera_distance = length(to_camera);

		/// [EN] A camera inside the cap sphere sees no silhouette; the end stays at the cap center.
		if (camera_distance <= r)
		{
			ends[cap_index] = cap_center;
			continue;
		}

		float3 view_direction = to_camera / camera_distance;
		float3 circle_center = cap_center + view_direction * (r * r / camera_distance);
		float circle_radius = r * sqrt(camera_distance * camera_distance - r * r) / camera_distance;

		float3 u = cross(float3(0.0, 1.0, 0.0), view_direction);
		u = (dot(u, u) > 1.0e-8) ? normalize(u) : float3(1.0, 0.0, 0.0);

		ends[cap_index] = circle_center + u * (side * circle_radius);
	}

	a = ends[0];
	b = ends[1];
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

	/// [EN] Rings at the bottom and top; angle 0 lies on +X and the ring turns toward +Z.
	if (index < 2 * COLLIDER_RING_SEGMENTS)
	{
		float y = (index < COLLIDER_RING_SEGMENTS) ? -h : h;
		uint segment = index % COLLIDER_RING_SEGMENTS;
		float angle0 = (float(segment) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		float angle1 = (float(segment + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle0), y, r * sin(angle0));
		b = float3(r * cos(angle1), y, r * sin(angle1));
		return;
	}
	index -= 2 * COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed side lines.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle), -h, r * sin(angle));
		b = float3(r * cos(angle), h, r * sin(angle));
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
	a = float3(r * cos(angle), -h, r * sin(angle));
	b = float3(r * cos(angle), h, r * sin(angle));
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

	/// [EN] Base ring; angle 0 lies on +X and the ring turns toward +Z.
	if (index < COLLIDER_RING_SEGMENTS)
	{
		float angle0 = (float(index) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		float angle1 = (float(index + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle0), -h, r * sin(angle0));
		b = float3(r * cos(angle1), -h, r * sin(angle1));
		return;
	}
	index -= COLLIDER_RING_SEGMENTS;

	/// [EN] Fixed lines from the base to the apex.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = float3(r * cos(angle), -h, r * sin(angle));
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

	float tangent_angle = base_angle + side * tangent_offset;
	a = float3(r * cos(tangent_angle), -h, r * sin(tangent_angle));
	b = apex;
}

/**
* [EN]
* One of the 4 edges of a rectangle on the XY plane, centered on the
* origin, for canvas colliders. dimensions.xy holds the half extents.
*/
void GetRectLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	float hx = dimensions.x;
	float hy = dimensions.y;

	/// [EN] Corners in order around the rectangle, so edge n joins corner n to the next one.
	float3 corners[4] =
	{
		float3(-hx, -hy, 0.0), float3(hx, -hy, 0.0),
		float3(hx, hy, 0.0), float3(-hx, hy, 0.0)
	};

	/// [EN] The last edge wraps from the fourth corner back to the first.
	a = corners[line_index];
	b = corners[(line_index + 1) % 4];
}

/**
* [EN]
* One segment of a circle on the XY plane, centered on the origin, for
* canvas colliders. dimensions.x is the radius.
*/
void GetCircleLine(float3 dimensions, uint line_index, out float3 a, out float3 b)
{
	/// [EN] The circle is split into COLLIDER_RING_SEGMENTS equal arcs, each drawn as a straight line; angle 0 lies on +X and the circle turns toward +Y.
	float r = dimensions.x;
	float angle0 = (float(line_index) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	float angle1 = (float(line_index + 1) / float(COLLIDER_RING_SEGMENTS)) * COLLIDER_TWO_PI;
	a = float3(r * cos(angle0), r * sin(angle0), 0.0);
	b = float3(r * cos(angle1), r * sin(angle1), 0.0);
}

/**
* [EN]
* Single line from the origin to dimensions.
*/
void GetSegmentLine(float3 dimentions, out float3 a, out float3 b)
{
    a = float3(0.0, 0.0, 0.0);
	b = dimentions;
}

/**
* [EN]
* Arrow from the origin to the tip at dimensions: a shaft up to the head,
* and a cone-shaped head drawn like the Cone shape (its base ring, four
* lines from the ring to the tip, and the two silhouette lines to the tip),
* so it reads as an arrow from any viewing angle.
*/
void GetArrowLine(float3 dimensions, float head_limit, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
{
	silhouette = false;

	float arrow_length = length(dimensions);

	/// [EN] A zero-length arrow has no direction and collapses to a point.
	if (arrow_length < 1.0e-6)
	{
		a = float3(0.0, 0.0, 0.0);
		b = a;
		return;
	}

	/// [EN] Two axes across the shaft; any pair perpendicular to it works, so Y is crossed with the shaft unless the shaft is nearly vertical, in which case X is used.
	float3 forward = dimensions / arrow_length;
	float3 side = normalize(cross(abs(forward.y) > 0.99 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0), forward));
	float3 up = cross(forward, side);

	/// [EN] The head is a fixed fraction of the arrow's length, kept within head_limit when one is given so a long arrow does not get a huge head; its base radius is a fixed fraction of the head's length.
	float head_length = arrow_length * COLLIDER_ARROW_HEAD_LENGTH_RATIO;
	if (head_limit > 0.0)
	{
		head_length = min(head_length, head_limit);
	}
	float head_width = head_length * COLLIDER_ARROW_HEAD_WIDTH_RATIO;
	float3 head_base = dimensions - forward * head_length;
	uint index = line_index;

	/// [EN] Shaft, stopping where the head's base begins.
	if (index == 0)
	{
		a = float3(0.0, 0.0, 0.0);
		b = head_base;
		return;
	}
	index -= 1;

	/// [EN] Base ring of the head; angle 0 lies on side and the ring turns toward up.
	if (index < COLLIDER_ARROW_RING_SEGMENTS)
	{
		float angle0 = (float(index) / float(COLLIDER_ARROW_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		float angle1 = (float(index + 1) / float(COLLIDER_ARROW_RING_SEGMENTS)) * COLLIDER_TWO_PI;
		a = head_base + (side * cos(angle0) + up * sin(angle0)) * head_width;
		b = head_base + (side * cos(angle1) + up * sin(angle1)) * head_width;
		return;
	}
	index -= COLLIDER_ARROW_RING_SEGMENTS;

	/// [EN] Fixed lines from the ring to the tip.
	if (index < COLLIDER_SIDE_LINE_COUNT)
	{
		float angle = (float(index) / float(COLLIDER_SIDE_LINE_COUNT)) * COLLIDER_TWO_PI;
		a = head_base + (side * cos(angle) + up * sin(angle)) * head_width;
		b = dimensions;
		return;
	}
	index -= COLLIDER_SIDE_LINE_COUNT;

	silhouette = true;

	/// [EN] Silhouette lines, found the same way as the Cone shape's but in the head's own frame (side, forward, up), with the tip as the apex and the base plane head_length behind it.
	float sign = (index == 0) ? 1.0 : -1.0;
	float3 tip_to_camera = local_camera - dimensions;
	float3 head_camera = float3(dot(tip_to_camera, side), dot(tip_to_camera, forward), dot(tip_to_camera, up));
	float base_angle;
	float tangent_offset;

	if (abs(head_camera.y) < 1.0e-5)
	{
		/// [EN] A camera level with the tip sees the head edge-on; the tangent points are a quarter turn from the camera direction.
		base_angle = atan2(head_camera.z, head_camera.x);
		tangent_offset = COLLIDER_PI * 0.5;
	}
	else
	{
		/// [EN] Where the line through the tip and the camera meets the base plane.
		float t = -head_length / head_camera.y;
		float2 base_point = head_camera.xz * t;
		float base_distance = length(base_point);

		/// [EN] A camera looking into the head from in front of the tip or behind the base sees no side silhouette.
		if (base_distance <= head_width)
		{
			a = dimensions;
			b = dimensions;
			return;
		}

		base_angle = atan2(base_point.y, base_point.x);
		tangent_offset = acos(head_width / base_distance);
	}

	float tangent_angle = base_angle + sign * tangent_offset;
	a = head_base + (side * cos(tangent_angle) + up * sin(tangent_angle)) * head_width;
	b = dimensions;
}

/**
* [EN]
* Local-space endpoints of one line of a shape, and whether that line
* belongs to the camera-facing silhouette. local_camera is the camera
* position in the shape's local space.
*/
void GetColliderLine(uint shape_kind, float3 dimensions, float head_length, float3 local_camera, uint line_index, out float3 a, out float3 b, out bool silhouette)
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
	else if (shape_kind == COLLIDER_SHAPE_SEGMENT)
	{
		GetSegmentLine(dimensions, a, b);
	}
	else if (shape_kind == COLLIDER_SHAPE_ARROW)
	{
		GetArrowLine(dimensions, head_length, local_camera, line_index, a, b, silhouette);
	}
}

#endif // __COLLIDER_LINE_HLSL__
