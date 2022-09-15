#include "map.h"
#include "body.h"
#include "forces.h"
#include "math.h"
#include "shapes.h"
#include "physics_constants.h"
#include "global_constants.h"
#include <stdio.h>
#include <stdlib.h>

const double WALL_THICKNESS = 10;
const double CURVE_THICKNESS = 50;
const double GRASS_THICKNESS = 100;
const double FINISH_WIDTH = 5;
const rgba_color_t WALL_COLOR = (rgba_color_t){ 0, 0.5, 1, 0.2 };
const rgba_color_t GRASS_COLOR = (rgba_color_t){ 0, 1, 0, 0.5 };

struct map {
	list_t *objects;
	double scaling;
	const char *img;
	map_type_t map_type;
	vector_t spawn;
};

double map_get_scaling(map_t *map) {
	return map->scaling;
}

void add_curve_clockwise(map_t *map, list_t *objects, double *vectors, size_t points) {
	//creates a rectangle for every 2 points in a curve
	for (size_t i = 0; i < points - 2; i++) {
		double a = *((vectors + i * 2) + 0);
		double b = *((vectors + i * 2) + 1);
		double x = *((vectors + i * 2) + 4);
		double y = *((vectors + i * 2) + 5);
		list_t *polygon = list_init(4, free);
		vector_t *one = malloc(sizeof(vector_t));
		vector_t *two = malloc(sizeof(vector_t));
		vector_t *three = malloc(sizeof(vector_t));
		vector_t *four = malloc(sizeof(vector_t));

		*one = vec_multiply(map->scaling, (vector_t){ a, b });
		*two = vec_multiply(map->scaling, (vector_t){ x, y });
		//make rectangle by using clockwise normal vector
		*three = vec_add(*two,
			vec_multiply(CURVE_THICKNESS, vec_unit_vec((vector_t){ b - y, x - a })));
		*four = vec_add(*one,
			vec_multiply(CURVE_THICKNESS, vec_unit_vec((vector_t){ b - y, x - a })));

		list_add(polygon, one);
		list_add(polygon, two);
		list_add(polygon, three);
		list_add(polygon, four);

		body_t *block = body_init_with_info(polygon, INFINITY,
			(rgba_color_t){ 0, 0.5, 1, 0.5 }, make_type_info(WALL), free);
		list_add(objects, block);

		//give collision info based on orientation of each rectangle
		collision_ref_t *info = malloc(sizeof(*info));
		*info = (collision_ref_t){ vec_magnitude(vec_subtract(*one, *three)) / 2, false,
			false, vec_unit_vec((vector_t){ y - b, a - x }), WALL };
		body_set_info(block, (void *)info);
	}
}

void add_curve_counterclockwise(
	map_t *map, list_t *objects, double *vectors, size_t points) {
		//create rectangle for every 2 points in counterclockwise curve
	for (size_t i = 0; i < points - 2; i++) {
		double a = *((vectors + i * 2) + 0);
		double b = *((vectors + i * 2) + 1);
		double x = *((vectors + i * 2) + 4);
		double y = *((vectors + i * 2) + 5);
		list_t *polygon = list_init(4, free);
		vector_t *one = malloc(sizeof(vector_t));
		vector_t *two = malloc(sizeof(vector_t));
		vector_t *three = malloc(sizeof(vector_t));
		vector_t *four = malloc(sizeof(vector_t));

		*one = vec_multiply(map->scaling, (vector_t){ a, b });
		*two = vec_multiply(map->scaling, (vector_t){ x, y });
		//make rectangle based on normal vector
		*three = vec_add(*two,
			vec_multiply(CURVE_THICKNESS, vec_unit_vec((vector_t){ y - b, a - x })));
		*four = vec_add(*one,
			vec_multiply(CURVE_THICKNESS, vec_unit_vec((vector_t){ y - b, a - x })));

		list_add(polygon, one);
		list_add(polygon, two);
		list_add(polygon, three);
		list_add(polygon, four);

		body_t *block = body_init_with_info(polygon, INFINITY,
			(rgba_color_t){ 0, 0.5, 1, 0.5 }, make_type_info(WALL), free);
		list_add(objects, block);

		collision_ref_t *info = malloc(sizeof(collision_ref_t));
		*info = (collision_ref_t){ vec_magnitude(vec_subtract(*one, *three)) / 2, false,
			false, vec_unit_vec((vector_t){ y - b, a - x }), WALL };
		body_set_info(block, (void *)info);
	}
}

void add_grass_curve_counterclockwise(
	map_t *map, list_t *objects, double *vectors, size_t points) {
	for (size_t i = 0; i < points - 2; i++) {
		double a = *((vectors + i * 2) + 0);
		double b = *((vectors + i * 2) + 1);
		double x = *((vectors + i * 2) + 4);
		double y = *((vectors + i * 2) + 5);
		list_t *polygon = list_init(4, free);
		vector_t *one = malloc(sizeof(vector_t));
		vector_t *two = malloc(sizeof(vector_t));
		vector_t *three = malloc(sizeof(vector_t));
		vector_t *four = malloc(sizeof(vector_t));

		*one = vec_multiply(map->scaling, (vector_t){ a, b });
		*two = vec_multiply(map->scaling, (vector_t){ x, y });
		*three = vec_add(*two,
			vec_multiply(GRASS_THICKNESS*2, vec_unit_vec((vector_t){ y - b, a - x })));
		*four = vec_add(*one,
			vec_multiply(GRASS_THICKNESS*2, vec_unit_vec((vector_t){ y - b, a - x })));

		list_add(polygon, one);
		list_add(polygon, two);
		list_add(polygon, three);
		list_add(polygon, four);

		body_t *block = body_init_with_info(polygon, INFINITY,
			(rgba_color_t){ 0, 1, 0, 0.5 }, make_type_info(GRASS), free);
		list_add(objects, block);

		collision_ref_t *info = malloc(sizeof(collision_ref_t));
		*info = (collision_ref_t){ vec_magnitude(vec_subtract(*one, *three)) / 2, false,
			false, vec_unit_vec((vector_t){ y - b, a - x }), GRASS };
		body_set_info(block, (void *)info);
	}
}

void add_grass_curve_clockwise(
	map_t *map, list_t *objects, double *vectors, size_t points) {
	for (size_t i = 0; i < points - 2; i++) {
		double a = *((vectors + i * 2) + 0);
		double b = *((vectors + i * 2) + 1);
		double x = *((vectors + i * 2) + 4);
		double y = *((vectors + i * 2) + 5);
		list_t *polygon = list_init(4, free);
		vector_t *one = malloc(sizeof(vector_t));
		vector_t *two = malloc(sizeof(vector_t));
		vector_t *three = malloc(sizeof(vector_t));
		vector_t *four = malloc(sizeof(vector_t));

		*one = vec_multiply(map->scaling, (vector_t){ a, b });
		*two = vec_multiply(map->scaling, (vector_t){ x, y });
		*three = vec_add(*two,
			vec_multiply(GRASS_THICKNESS * 2, vec_unit_vec((vector_t){ b - y, x - a })));
		*four = vec_add(*one,
			vec_multiply(GRASS_THICKNESS * 2, vec_unit_vec((vector_t){ b - y, x - a })));

		list_add(polygon, one);
		list_add(polygon, two);
		list_add(polygon, three);
		list_add(polygon, four);

		body_t *block = body_init_with_info(polygon, INFINITY,
			(rgba_color_t){ 0, 1, 0, 0.5 }, make_type_info(GRASS), free);
		list_add(objects, block);

		collision_ref_t *info = malloc(sizeof(collision_ref_t));
		*info = (collision_ref_t){ vec_magnitude(vec_subtract(*one, *three)) / 2, false,
			false, vec_unit_vec((vector_t){ y - b, a - x }), GRASS };
		body_set_info(block, (void *)info);
	}
}

void add_walls(list_t *objects) {
	//add four bounding walls 
	body_t *right_wall = body_init_with_info(
		make_rectangle((vector_t){ SCENE_WIDTH / 2, SCENE_HEIGHT + (WALL_THICKNESS / 2) },
			SCENE_WIDTH, WALL_THICKNESS),
		INFINITY, (rgba_color_t){}, make_type_info(WALL), free);

	collision_ref_t *info_right = malloc(sizeof(*info_right));
	*info_right = (collision_ref_t){ SCENE_WIDTH, false, false,
		vec_unit_vec((vector_t){ 1, 0 }), WALL };
	body_set_info(right_wall, (void *)info_right);

	body_t *left_wall = body_init_with_info(
		make_rectangle((vector_t){ -WALL_THICKNESS, SCENE_HEIGHT / 2 }, WALL_THICKNESS,
			SCENE_HEIGHT),
		INFINITY, (rgba_color_t){}, make_type_info(WALL), free);

	collision_ref_t *info_left = malloc(sizeof(*info_left));
	*info_left = (collision_ref_t){ SCENE_WIDTH, false, false,
		vec_unit_vec((vector_t){ 1, 0 }), WALL };
	body_set_info(left_wall, (void *)info_left);

	body_t *top_wall = body_init_with_info(
		make_rectangle((vector_t){ SCENE_WIDTH + (WALL_THICKNESS / 2), SCENE_HEIGHT / 2 },
			WALL_THICKNESS, SCENE_HEIGHT),
		INFINITY, (rgba_color_t){}, make_type_info(WALL), free);

	collision_ref_t *info_top = malloc(sizeof(*info_top));
	*info_top = (collision_ref_t){ SCENE_WIDTH, false, false,
		vec_unit_vec((vector_t){ 0, 1 }), WALL };
	body_set_info(top_wall, (void *)info_top);

	body_t *bottom_wall = body_init_with_info(
		make_rectangle((vector_t){ SCENE_WIDTH / 2, -WALL_THICKNESS / 2 }, SCENE_WIDTH,
			WALL_THICKNESS),
		INFINITY, (rgba_color_t){}, make_type_info(WALL), free);

	collision_ref_t *info_bottom = malloc(sizeof(*info_bottom));
	*info_bottom = (collision_ref_t){ SCENE_WIDTH, false, false,
		vec_unit_vec((vector_t){ 0, 1 }), WALL };
	body_set_info(bottom_wall, (void *)info_bottom);

	list_add(objects, top_wall);
	list_add(objects, bottom_wall);
	list_add(objects, left_wall);
	list_add(objects, right_wall);
}


void add_map1(map_t *map, list_t *objects) {
	// add grass block in middle

	double points1[][2] = { { 652.93, 817.22 }, { 1621.79, 819.33 }, { 1672.34, 808.80 },
		{ 1720.78, 791.95 }, { 1756.59, 766.67 }, { 1786.07, 741.40 },
		{ 1807.13, 709.80 }, { 1826.09, 678.21 }, { 1838.73, 640.30 },
		{ 1849.26, 606.60 }, { 1849.26, 577.11 }, { 1845.05, 530.77 },
		{ 1838.73, 497.07 }, { 1821.88, 463.38 }, { 1800.82, 425.46 },
		{ 1773.44, 387.55 }, { 1743.95, 362.28 }, { 1706.04, 343.32 },
		{ 1668.12, 328.58 }, { 1621.79, 322.26 }, { 667.68, 324.36 }, { 619.24, 332.79 },
		{ 575.00, 353.85 }, { 539.20, 379.13 }, { 511.82, 406.51 }, { 484.44, 444.42 },
		{ 465.48, 484.44 }, { 450.74, 545.52 }, { 450.74, 602.39 }, { 459.16, 642.41 },
		{ 473.91, 680.32 }, { 494.97, 709.80 }, { 509.71, 739.29 }, { 543.41, 768.78 },
		{ 591.85, 796.16 } };

	body_t *block1 =
		body_init_with_info(make_polygon((double *)points1, 35, map->scaling), INFINITY,
			GRASS_COLOR, make_type_info(GRASS), free);

	collision_ref_t *info_1 = malloc(sizeof(*info_1));
	*info_1 = (collision_ref_t){ SCENE_WIDTH, false, false,
		vec_unit_vec((vector_t){ 0, 1 }), GRASS };
	body_set_info(block1, (void *)info_1);
	list_add(objects, block1);

	// outside walls
	double points2[][2] = { { 1638.64, 1019.42 }, { 1684.97, 1011.00 },
		{ 1727.10, 1002.57 }, { 1771.33, 987.83 }, { 1819.77, 964.66 },
		{ 1855.58, 939.38 }, { 1893.49, 914.11 }, { 1925.08, 878.30 },
		{ 1962.99, 844.60 }, { 1988.27, 800.37 }, { 2013.54, 751.93 },
		{ 2028.29, 707.70 }, { 2047.24, 648.72 }, { 2053.56, 600.28 },
		{ 2053.56, 549.73 }, { 2049.35, 499.18 }, { 2034.60, 448.63 },
		{ 2015.65, 406.51 }, { 2000.91, 358.06 }, { 1975.63, 326.47 },
		{ 1954.57, 294.88 }, { 1933.51, 269.60 }, { 1933.51, 269.60 },
		{ 1906.13, 242.22 }, { 1870.32, 210.63 }, { 1828.20, 181.14 },
		{ 1792.39, 168.50 }, { 1758.69, 151.65 }, { 1722.89, 134.80 },
		{ 1674.44, 126.38 }, { 1613.36, 122.17 }, { 671.89, 120.06 }, { 627.66, 126.38 },
		{ 585.54, 134.80 }, { 543.41, 145.33 }, { 499.18, 168.50 }, { 450.74, 197.99 },
		{ 408.61, 227.48 }, { 366.49, 271.71 }, { 332.79, 309.62 }, { 305.41, 358.06 },
		{ 278.03, 410.72 }, { 259.07, 469.69 }, { 248.54, 522.35 }, { 252.76, 583.43 },
		{ 252.76, 638.19 }, { 261.18, 684.53 }, { 282.24, 726.65 }, { 303.30, 781.42 },
		{ 328.58, 827.75 }, { 358.07, 867.77 }, { 391.77, 897.26 }, { 421.25, 918.32 },
		{ 461.27, 952.02 }, { 501.29, 979.40 }, { 549.73, 998.36 }, { 602.39, 1011.00 },
		{ 665.57, 1021.53 },{ 665.57, 1021.53 }, { 1638.64, 1019.42 }, { 1638.64, 1019.42 } };
	add_curve_clockwise(map, objects, points2, 61);

	// inner walls
	double points3[][2] = { { 1615.5, 754.1 }, { 1592.3, 754.1 }, { 709.8, 754.1 },
		{ 680.3, 754.1 }, { 652.9, 749.9 }, { 631.9, 741.5 }, { 594.0, 726.8 },
		{ 579.2, 714.1 }, { 558.1, 690.9 }, { 537.1, 659.4 }, { 526.6, 625.7 },
		{ 518.1, 598.3 }, { 518.1, 564.6 }, { 520.2, 535.1 }, { 528.7, 511.9 },
		{ 537.1, 484.5 }, { 549.7, 467.7 }, { 564.5, 448.7 }, { 583.4, 434.0 },
		{ 600.3, 421.4 }, { 625.5, 404.5 }, { 646.6, 396.1 }, { 671.9, 389.8 },
		{ 703.5, 389.8 }, { 726.6, 389.8 }, { 1596.5, 389.8 }, { 1617.6, 391.9 },
		{ 1638.6, 394.0 }, { 1663.9, 400.3 }, { 1680.8, 406.6 }, { 1706.0, 415.0 },
		{ 1725.0, 438.2 }, { 1744.0, 461.4 }, { 1765.0, 488.7 }, { 1773.4, 516.1 },
		{ 1781.9, 560.4 }, { 1781.9, 589.8 }, { 1777.7, 621.4 }, { 1769.2, 646.7 },
		{ 1756.6, 674.1 }, { 1735.5, 693.1 }, { 1718.7, 714.1 }, { 1695.5, 731.0 },
		{ 1670.2, 745.7 }, { 1640.8, 752.0 }, { 1615.5, 754.1 }, { 1615.5, 754.1 } };

	add_curve_clockwise(map, objects, points3, 47);

	body_t *finish_begin = body_init_with_info(
		make_rectangle((vector_t){ 1880 + FINISH_WIDTH * 3, 1495}, FINISH_WIDTH,
			400 * map->scaling),
		INFINITY, (rgba_color_t){ 1, 0, 0, 0 }, make_type_info(FINISH_BEGIN), free);

	collision_ref_t *info1 = malloc(sizeof(collision_ref_t));
	*info1 = (collision_ref_t){ {0,0}, false, false, {0,0}, FINISH_BEGIN };
	body_set_info(finish_begin, (void *)info1);

	body_t *finish_end = body_init_with_info(
		make_rectangle((vector_t){ 1880 , 1495}, FINISH_WIDTH,
			400 * map->scaling),
		INFINITY, (rgba_color_t){ 1, 0, 0, 0 }, make_type_info(FINISH_END), free);

	collision_ref_t *info2 = malloc(sizeof(collision_ref_t));
	*info2 = (collision_ref_t){ {0,0}, false, false, {0,0}, FINISH_END };
	body_set_info(finish_end, (void *)info2);

	list_add(objects, finish_begin);
	list_add(objects, finish_end);
}

void add_map2(map_t *map, list_t *objects) {
	// outside walls
	double points0[][2] = { { 2359.3, 836.5 }, { 2408.1, 836.5 }, { 3649.6, 810.2 },
		{ 3705.9, 810.2 }, { 3777.2, 806.4 }, { 3848.4, 798.9 }, { 3908.5, 780.2 },
		{ 3961.0, 765.2 }, { 4002.2, 727.7 }, { 4058.5, 667.7 }, { 4077.2, 603.9 },
		{ 4077.2, 517.6 }, { 4077.2, 457.6 }, { 4077.2, 397.6 }, { 4077.2, 322.6 },
		{ 4062.2, 255.1 }, { 4039.7, 202.5 }, { 4013.5, 157.5 }, { 3976.0, 120.0 },
		{ 3919.7, 71.3 }, { 3844.7, 52.5 }, { 3777.2, 37.5 }, { 3705.9, 33.8 },
		{ 3619.6, 37.5 }, { 3548.4, 52.5 }, { 3492.1, 63.8 }, { 3439.6, 63.8 },
		{ 2021.7, 67.5 }, { 1984.2, 56.3 }, { 1905.5, 56.3 }, { 626.4, 37.5 },
		{ 588.9, 37.5 }, { 506.4, 52.5 }, { 435.1, 108.8 }, { 393.8, 146.3 },
		{ 356.3, 187.5 }, { 318.8, 255.1 }, { 307.6, 322.6 }, { 296.3, 382.6 },
		{ 277.6, 476.4 }, { 277.6, 547.6 }, { 277.6, 630.2 }, { 292.6, 708.9 },
		{ 303.8, 791.4 }, { 307.6, 862.7 }, { 303.8, 964.0 }, { 240.1, 1027.7 },
		{ 180.0, 1095.3 }, { 112.5, 1140.3 }, { 82.5, 1204.0 }, { 52.5, 1305.3 },
		{ 41.3, 1402.8 }, { 45.0, 1507.9 }, { 41.3, 1597.9 }, { 41.3, 1691.7 },
		{ 37.5, 1762.9 }, { 37.5, 1837.9 }, { 60.0, 1898.0 }, { 97.5, 1928.0 },
		{ 172.5, 1954.2 }, { 270.1, 1980.5 }, { 390.1, 2018.0 }, { 461.4, 2018.0 },
		{ 532.6, 2036.7 }, { 596.4, 2040.5 }, { 855.2, 2048.0 }, { 870.2, 2021.7 },
		{ 889.0, 1995.5 }, { 1016.5, 1657.9 }, { 1024.0, 1605.4 }, { 1035.3, 1567.9 },
		{ 1069.0, 1522.9 }, { 1087.8, 1462.9 }, { 1121.5, 1432.8 }, { 1162.8, 1399.1 },
		{ 1717.9, 990.2 }, { 1751.7, 971.5 }, { 1804.2, 971.5 }, { 1853.0, 982.7 },
		{ 1898.0, 994.0 }, { 1954.2, 1012.7 }, { 2003.0, 1009.0 }, { 2048.0, 997.7 },
		{ 2104.3, 986.5 }, { 2156.8, 964.0 }, { 2228.0, 937.7 }, { 2273.1, 907.7 },
		{ 2314.3, 870.2 } };
	add_curve_clockwise(map, objects, points0, 88);

	// inside walls
	double points1[][2] ={{3469.6, 202.5},{2018.0, 198.8},{2003.0, 198.8},{1980.5, 243.8},{1946.7, 243.8},{1920.5, 247.6},{1898.0, 247.6},{1868.0, 251.3},{1834.2, 255.1},{1807.9, 262.6},{1777.9, 277.6},{1759.2, 300.1},{1747.9, 333.8},{1717.9, 382.6},{1691.7, 416.4},{1672.9, 446.4},{1646.7, 472.6},{1627.9, 480.1},{1582.9, 498.9},{1552.9, 502.6},{1515.4, 506.4},{1477.9, 495.1},{1447.9, 483.9},{1421.6, 472.6},{1387.8, 465.1},{1106.5, 292.6},{1061.5, 277.6},{1024.0, 262.6},{979.0, 262.6},{941.5, 258.8},{738.9, 262.6},{705.2, 273.8},{660.2, 300.1},{637.7, 311.3},{618.9, 348.8},{600.1, 375.1},{577.6, 401.3},{562.6, 431.4},{540.1, 476.4},{532.6, 513.9},{521.4, 562.6},{521.4, 611.4},{536.4, 997.7},{536.4, 1042.8},{536.4, 1091.5},{532.6, 1129.0},{528.9, 1166.5},{521.4, 1200.3},{506.4, 1260.3},{495.1, 1297.8},{472.6, 1320.3},{438.9, 1346.6},{401.3, 1376.6},{367.6, 1391.6},{333.8, 1406.6},{326.3, 1444.1},{311.3, 1492.9},{281.3, 1601.6},{277.6, 1635.4},{277.6, 1684.2},{303.8, 1710.4},{330.1, 1721.7},{607.6, 1792.9},{637.7, 1800.4},{675.2, 1804.2},{712.7, 1796.7},{731.4, 1777.9},{753.9, 1755.4},{772.7, 1725.4},{783.9, 1691.7},{798.9, 1665.4},{813.9, 1627.9},{821.5, 1586.6},{832.7, 1541.6},{847.7, 1496.6},{874.0, 1451.6},{881.5, 1417.8},{907.7, 1387.8},{930.2, 1346.6},{949.0, 1320.3},{967.7, 1297.8},{990.2, 1275.3},{1012.7, 1256.6},{1050.3, 1222.8},{1084.0, 1192.8},{1114.0, 1162.8},{1155.3, 1136.5},{1601.6, 832.7},{1635.4, 806.4},{1687.9, 787.7},{1740.4, 757.7},{1781.7, 731.4},{1838.0, 712.7},{1879.2, 705.2},{1920.5, 697.7},{1973.0, 697.7},{2018.0, 708.9},{2051.8, 712.7},{2089.3, 712.7},{2134.3, 712.7},{2153.0, 697.7},{2190.5, 652.7},{2213.0, 618.9},{2235.5, 592.6},{2273.1, 562.6},{2303.1, 543.9},{2370.6, 540.1},{2426.8, 543.9},{3653.4, 540.1},{3690.9, 543.9},{3724.7, 540.1},{3754.7, 532.6},{3780.9, 506.4},{3780.9, 483.9},{3780.9, 446.4},{3780.9, 412.6},{3765.9, 375.1},{3735.9, 322.6},{3702.2, 292.6},{3638.4, 262.6},{3597.1, 240.1},{3540.9, 225.1},{3510.9, 210.1}};
	add_curve_counterclockwise(map, objects, points1, 123);

	// inside grass
	double points2[][2] = {{2430.6, 588.9},{2456.9, 588.9},{3683.4, 585.1},{3698.4, 585.1},{3724.7, 581.4},{3750.9, 577.6},{3773.4, 566.4},{3795.9, 551.4},{3814.7, 528.9},{3822.2, 491.4},{3829.7, 465.1},{3829.7, 423.9},{3818.4, 390.1},{3803.4, 352.6},{3780.9, 318.8},{3750.9, 296.3},{3720.9, 273.8},{3687.2, 247.6},{3653.4, 232.6},{3600.9, 217.6},{3540.9, 210.1},{3488.4, 210.1},{1838.0, 210.1},{1770.4, 217.6},{1736.7, 228.8},{1717.9, 277.6},{1702.9, 326.3},{1680.4, 352.6},{1657.9, 371.3},{1627.9, 393.8},{1594.1, 397.6},{1552.9, 408.8},{1515.4, 397.6},{1470.4, 382.6},{1440.4, 375.1},{1402.8, 356.3},{1369.1, 341.3},{1324.1, 322.6},{1297.8, 300.1},{1275.3, 277.6},{1252.8, 262.6},{1226.6, 240.1},{1185.3, 217.6},{1147.8, 210.1},{1087.8, 210.1},{825.2, 206.3},{787.7, 206.3},{723.9, 213.8},{690.2, 225.1},{652.7, 236.3},{637.7, 255.1},{615.1, 277.6},{592.6, 303.8},{555.1, 333.8},{532.6, 367.6},{510.1, 412.6},{502.6, 446.4},{495.1, 472.6},{476.4, 540.1},{472.6, 592.6},{491.4, 967.7},{491.4, 1024.0},{491.4, 1057.8},{487.6, 1102.8},{468.9, 1147.8},{435.1, 1177.8},{397.6, 1215.3},{352.6, 1226.5},{322.6, 1241.6},{292.6, 1275.3},{292.6, 1312.8},{232.6, 1624.1},{217.6, 1684.2},{217.6, 1732.9},{247.6, 1747.9},{285.1, 1751.7},{326.3, 1762.9},{686.4, 1868.0},{720.2, 1868.0},{742.7, 1871.7},{742.7, 1871.7},{757.7, 1853.0},{772.7, 1822.9},{907.7, 1470.4},{919.0, 1440.4},{937.7, 1410.3},{952.7, 1387.8},{952.7, 1387.8},{975.2, 1357.8},{997.7, 1316.6},{1024.0, 1297.8},{1500.4, 934.0},{1537.9, 911.5},{1582.9, 877.7},{1627.9, 862.7},{1650.4, 844.0},{1680.4, 825.2},{1714.2, 821.5},{1755.4, 821.5},{1815.4, 821.5},{1868.0, 832.7},{1901.7, 844.0},{1931.7, 851.5},{1973.0, 862.7},{2006.7, 862.7},{2048.0, 851.5},{2074.3, 832.7},{2108.0, 813.9},{2138.0, 787.7},{2160.5, 757.7},{2179.3, 727.7},{2194.3, 705.2},{2213.0, 682.7},{2231.8, 663.9},{2254.3, 648.9},{2280.6, 622.7},{2310.6, 615.1},{2348.1, 607.6},{2381.8, 592.6},{2411.8, 585.1}};

		
	add_grass_curve_counterclockwise(map, objects, points2, 120);

	// outside grass
	double points3[][2] = { { 2426.8, 705.2 }, { 2453.1, 701.4 }, { 3675.9, 701.4 },
		{ 3705.9, 701.4 }, { 3739.7, 697.7 }, { 3773.4, 693.9 }, { 3818.4, 675.2 },
		{ 3855.9, 652.7 }, { 3882.2, 633.9 }, { 3912.2, 596.4 }, { 3931.0, 566.4 },
		{ 3938.5, 517.6 }, { 3946.0, 480.1 }, { 3946.0, 427.6 }, { 3934.7, 382.6 },
		{ 3919.7, 341.3 }, { 3904.7, 307.6 }, { 3889.7, 273.8 }, { 3863.4, 236.3 },
		{ 3822.2, 202.5 }, { 3788.4, 183.8 }, { 3762.2, 161.3 }, { 3717.2, 138.8 },
		{ 3653.4, 116.3 }, { 3585.9, 97.5 }, { 3503.4, 90.0 }, { 3465.8, 86.3 },
		{ 1811.7, 90.0 }, { 1785.4, 93.8 }, { 1736.7, 97.5 }, { 1691.7, 116.3 },
		{ 1657.9, 142.5 }, { 1639.2, 172.5 }, { 1627.9, 195.0 }, { 1616.6, 225.1 },
		{ 1612.9, 247.6 }, { 1594.1, 277.6 }, { 1571.6, 281.3 }, { 1537.9, 281.3 },
		{ 1511.6, 273.8 }, { 1462.9, 258.8 }, { 1417.8, 243.8 }, { 1282.8, 142.5 },
		{ 1241.6, 112.5 }, { 1185.3, 97.5 }, { 1117.8, 93.8 }, { 765.2, 93.8 },
		{ 738.9, 93.8 }, { 697.7, 105.0 }, { 667.7, 112.5 }, { 630.2, 127.5 },
		{ 585.1, 157.5 }, { 558.9, 180.0 }, { 517.6, 210.0 }, { 487.6, 247.6 },
		{ 457.6, 288.8 }, { 438.9, 337.6 }, { 412.6, 367.6 }, { 397.6, 416.3 },
		{ 386.3, 457.6 }, { 367.6, 502.6 }, { 360.1, 573.9 }, { 360.1, 622.7 },
		{ 382.6, 994.0 }, { 382.6, 1020.2 }, { 382.6, 1061.5 }, { 360.1, 1091.5 },
		{ 337.6, 1102.8 }, { 292.6, 1117.8 }, { 251.3, 1132.8 }, { 225.1, 1166.5 },
		{ 206.3, 1204.0 }, { 187.5, 1256.6 }, { 105.0, 1672.9 }, { 105.0, 1717.9 },
		{ 116.3, 1766.7 }, { 127.5, 1807.9 }, { 150.0, 1830.4 }, { 183.8, 1841.7 },
		{ 228.8, 1860.5 }, { 648.9, 1991.7 }, { 686.4, 1988.0 }, { 731.4, 1988.0 },
		{ 765.2, 1988.0 }, { 787.7, 1973.0 }, { 825.2, 1958.0 }, { 859.0, 1924.2 },
		{ 889.0, 1890.5 }, { 889.0, 1864.2 }, { 889.0, 1864.2 }, { 1001.5, 1567.9 },
		{ 1001.5, 1552.9 }, { 1016.5, 1534.1 }, { 1031.5, 1492.9 }, { 1057.8, 1455.4 },
		{ 1091.5, 1410.3 }, { 1125.3, 1369.1 }, { 1147.8, 1350.3 }, { 1680.4, 967.7 },
		{ 1710.4, 941.5 }, { 1755.4, 937.7 }, { 1796.7, 937.7 }, { 1838.0, 949.0 },
		{ 1894.2, 956.5 }, { 1950.5, 964.0 }, { 1976.7, 975.2 }, { 2018.0, 975.2 },
		{ 2051.8, 964.0 }, { 2089.3, 952.7 }, { 2130.5, 937.7 }, { 2168.0, 907.7 },
		{ 2201.8, 885.2 }, { 2231.8, 851.5 }, { 2261.8, 817.7 }, { 2288.1, 787.7 },
		{ 2321.8, 742.7 }, { 2363.1, 727.7 }, { 2393.1, 712.7 }, { 2426.8, 705.2 } };
	add_grass_curve_clockwise(map, objects, points3, 119);

	// add the finish lines, begin and end
	body_t *finish_begin = body_init_with_info(
		make_rectangle((vector_t){ 2438 * map->scaling + 3 * FINISH_WIDTH, 248 * map->scaling }, FINISH_WIDTH,
			400 * map->scaling),
		INFINITY, (rgba_color_t){ 1, 0, 0, 0 }, make_type_info(FINISH_BEGIN), free);
	collision_ref_t *info1 = malloc(sizeof(collision_ref_t));
	*info1 = (collision_ref_t){ {0,0}, false, false, {0,0}, FINISH_BEGIN };
	body_set_info(finish_begin, (void *)info1);

	body_t *finish_end = body_init_with_info(
		make_rectangle((vector_t){ 2438 * map->scaling, 248 * map->scaling }, FINISH_WIDTH,
			400 * map->scaling),
		INFINITY, (rgba_color_t){ 1, 0, 0, 0 }, make_type_info(FINISH_END), free);
	collision_ref_t *info2 = malloc(sizeof(collision_ref_t));
	*info2 = (collision_ref_t){ {0,0}, false, false, {0,0}, FINISH_END };
	body_set_info(finish_end, (void *)info2);

	list_add(objects, finish_begin);
	list_add(objects, finish_end);
}


map_t *map_init(map_type_t type) {
	map_t *map = malloc(sizeof(*map));
	map->objects = list_init(0, NULL);
	map->map_type = type;
	map->img = NULL;

	// default
	map->scaling = 1;
	switch (type) {
		case MAP_ONE: {
			map->img = "assets/maps/map1.png";
			map->scaling = 2048.0 / 1148;
			add_map1(map, map->objects);
			map->spawn = (vector_t){ 2010, 1710 };

			break;
		}
		case MAP_TWO: {
			map->img = "assets/maps/map2.png";
			map->scaling = 1;
			add_map2(map, map->objects);
			map->spawn = (vector_t){ 2505, 126 };
			break;
		}
	}
	return map;
}

vector_t spawn_offset(map_type_t type, size_t idx) {
	switch (type) {
		case MAP_ONE: {
			double h_offset = 196;
			double h_offset_mid = 82;
			double v_offset = 80;

			double x_offset = 0;
			double y_offset = 0;

			x_offset += (idx % 2) * h_offset_mid + (idx / 2) * h_offset;
			y_offset -= (idx % 2) * v_offset;

			return (vector_t){ x_offset, y_offset };
		}
		case MAP_TWO: {
			double h_offset = 110;
			double h_offset_mid = 46;
			double v_offset = 45;

			double x_offset = 0;
			double y_offset = 0;

			x_offset += (idx % 2) * h_offset_mid + (idx / 2) * h_offset;
			y_offset += (idx % 2) * v_offset;

			return (vector_t){ x_offset, y_offset };
		}
	}
}

vector_t map_get_spawn(map_t *map, size_t idx) {
	return vec_add(map->spawn, spawn_offset(map->map_type, idx));
}

const char *map_get_image(map_t *map) { return map->img; }

list_t *map_get_objects(map_t *map) { return map->objects; }

map_type_t map_get_type(map_t *map) { return map->map_type; }