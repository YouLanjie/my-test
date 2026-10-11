/**
 * @file        r3d_rotate.c
 * @author      Chglish
 * @date        2026-07-12
 * @brief       一个简单的3d N体运动模拟程序，
 * 可任意选择天体跟踪操纵，内置仿真简易太阳系
 */

#include "libnbody/nbody.h"
#include <time.h>

static void print_qrh(bool no_page)
{
	(void)R"(
	(()"; // "  /* 由于vim语法高亮匹配问题，需要这个东西修正括号匹配 */
	const char *content = R"(（以下内容由AI生成，人类监制修正）
================================================================================
                   【 轨道飞行快速参考手册 QRH v1.0 】
================================================================================
索引 | 1. 全键位速查  2. UI信息解释与使用  3. 变轨黄金律  4. 霍曼转移自动化流程
================================================================================

[1] 全键位速查 (CAM/ENG/NAV/SYS)
--------------------------------------------------------------------------------
【视角控制 CAM】
 平移  : W(上) S(下) A(左) D(右)
 旋转  : h(左转) l(右转) j(下转) k(上转)   ← 1° 小幅度转动视线
 滚转  : H(左滚45°) L(右滚45°)             ← 大角度歪头（改变画面上方向）
         J(左滚1°)  K(右滚1°)              ← 小角度歪头（微调水平基准）
 大旋  : < (左摇头45°) > (右摇头45°)       ← 快速环顾四周
 推拉  : - (后退1) = (前进1)               ← 线性移动视点(天文尺度下不常用)
         + (瞬移至半距视角)                ← 使相机与目标距离减半（放大）
         _ (瞬移至倍距视角)                ← 使相机与目标距离翻倍（缩小）
 焦距  : 7 (缩小视场角) 8 (放大视场角)     ← 改变透视畸变(一般不建议调整)
 深度  : 9 (减半可视距离) 0 (加倍可视距离)

【引擎控制 ENG】 (须先按 f 锁定跟随天体)
 油门  : z(+1%) x(-1%)  Z(满油门/255)  X(急停/0)
 点火  : 空格(开关)  r(切换反向推力)
 手动机动 (沿视角方向，不常用，不推荐用) :
         n/N (加速 1x/10x)  b/B (减速 1x/10x)
 手动机动 (沿上下左右方向，用于微小径向/法向修正，不常用) :
         w(上) s(下) a(左) d(右)

【目标与导航 NAV】
 f : 选择驾驶/跟随天体 (自动切换视角并锁定)
 t : 选择最终目标天体 (**核心导航键**：设定后自动显示 Rp/Ra 及霍曼数据)
 F : 选择看向天体 (仅锁定视线方向，不参与导航计算。若看向自身则看向自身速度方向)
 T : 切换开关“视角随相对速度方向自动旋转”    ← 用于轨道面内自动对向飞行
 ? : 打开轨道计算辅助面板，计算查看更详细的轨道参数

【系统与调试 SYS】
 时间  : [ (减半) ] (加倍)  { (重置1倍) } (设定为3600倍)
 暂停  : p (切换)  . (单帧步进，一般不常用)
 后端  : Tab (轮换渲染后端)  ` (相机匹配屏幕尺寸)  ~ (重载当前渲染后端)
 显示  : i (坐标轴开/关)  I (辅助参考线开/关)
 日志  : " (分页查看航行日志)  | (导出所有天体原始数据)
 手册  : M (打开本手册)
 调试  : ' (帧率/负载显示开/关)
 清屏  : c (强制刷新画面)
 退出  : q/Q

================================================================================
[2] UI界面信息解释与使用
--------------------------------------------------------------------------------
【三条核心线的颜色含义】 (按 I 开启)
 黄线 (相对速度矢量) : 指向当前天体相对于参考系中心的速度方向。
 青线 (目标指向)     : 指向你按 t 设定的目标天体。
 灰线 (引力中心)     : 指向当前捕获你的参考系中心天体。

【主状态栏/HUD的解释】
-------------------------------
[T+0.1d, x1, :80%:, dv:0km/s] | 地球小卫星[地球] (5.729 km/s) Rp:11934.2km Ra:12066.5km
距月球 389861.1 km (-3.976 km/s) 33.5°(-29.6)°/8.5° L:283.81s [已暂停] (  2.1%/40fps)
-------------------------------
 - T+0.1d : 当前游戏持续时间
 - x1     : 当前游戏倍速，超过一定数值后会使用科学记数法
 - :80%:  : 油门显示，`:255%:`表示设定255油门但未启动，`[255%]`表示最大推力推进中
            若显示为`:-10%:`或`[-10%]`则表示推力与视线方向相反(使用r反转)
 - dv:0km/s     : 表示当前累计手动变速速度变化量，衡量机动高效程度
 - 地球小卫星   : 当前跟踪/操纵天体
 - [地球]       : 当前天体环绕天体，若变为`{地球}`则表示启用了T跟随速度方向旋转视角
 - (5.729 km/s) : 相对中心天体速度大小
 - Rp:11934.2km : 近地点，仅在设置了`t`目标后显示
 - Ra:12066.5km : 远地点，显示同近地点，油门启动时与近地点交换高度时会自动暂停
 - 距月球       : 当前设定的目标(该行的内容若无说明，均需要设置目标才显示)
 - 389861.1 km  : 相对目标的直线距离
 - (-3.976 km/s): 相对目标垂直速度，负数表示接近正数表示远离
 - 33.5°(-29.6)°/8.5° : 见下小节
 - L:283.81s    : 霍曼转移剩余加速时间 (仅在 |C|<10° 时显示，油门为0时示数为0)
 - [已暂停]     : 暂停状态，若没有设置t则会在第一行显示
 - (  2.1%/40fps) : 负载状况，用'打开查看，若超过100%表示超载，帧率会降低(暂不支持跳帧)

【霍曼转移角度参数诊断 (状态栏第二行中右端) 】 (仅当 t 目标与飞船共心时显示)
 显示格式 : `A(B)°/C°`
   A = 当前轨道面与目标天体轨道面的 绝对倾角 (越小越共面)。
   B = 速度方向与两轨道面交线方向的 夹角 (取值范围 0°~90°)。
   C = 距霍曼转移最佳点火点的 剩余角度 (0° 即为最佳时机)。

【倾角修正的几何原理与视角对齐法】
 改变倾角的最小 Δv 方向，是“当前速度向量”与“目标轨道面内速度向量”的向量差方向，
 机动前后的速度大小（速率）应当尽量保持不变。
 > 形象地说：若两速度向量等长，Δv 方向即等腰三角形的**底边方向**。

 ① 视角对齐轨道面（使用 hjkl）：
    利用 `h/j/k/l` 旋转视角，直到**屏幕边缘大致平行于灰线（引力中心）与青线（目标方向）所张成的平面**。
    （直观判断：让青灰线看起来处于同一条直线（水平或竖直最易判断），而不张开一定角度。）
    技巧：
    - 先转动视角尽可能让两条线对称分布在屏幕两侧
    - 再使用hj调整角度让两条线角度逐渐减小直至近似为一条直线即可

 ② 歪头构造等腰三角形（使用 HJKL 滚转）：
    使用 `H/L` 或 `J/K` 进行滚转（歪头），仔细调整，使得屏幕上的黄线与
    青线或灰线**在屏幕上恰好形成一个锐角等腰三角形**。
    （或者让"黄线"与"和黄线构成锐角的另一条线"在屏幕上对称分布在屏幕两侧）

 ③ 90°转向锁定最优推力方向（使用 `>` 或 `<`）：
    保持当前视角不变，**连续按两次 `>`（或两次 `<`）**。
    具体选择方向取决于黄线方位，黄线在左就按`<<`，在右就按`>>`
    该操作将视线在水平面内旋转 90°，此时：
    > 此时你的视线方向就已经精确指向了最小 Δv 的推力方向
    如果黄线与另一条线几乎重叠分不出来方向，可任选一边旋转后进入下一步试错

 ④ 启动推进与趋势验证：
    视线方向即推力方向。设定好推力，启动油门开始修正。
    如果不确定推力方向，可以：
    1. 先暂停，使用?观察相对倾角，记录数值
    2. 快速取消又恢复暂停，再用?观察，相对倾角减小了表明方向对了，否则使用r逆转推力方向
    修正时观察状态栏(`A(B)°/C°`)：
    - B值 增大 → 倾角 A 减小 → 方向正确，继续推进！
    - B值 减小 → 倾角 A 增大 → 方向反了，立即按 r 切换推力方向。
    当 B 值持续增大并趋近 90° 时，表明已将倾角压至本次修正的极限最小值。
    ⚠️ 超过 90° 继续推进会导致倾角反弹增大，必须在 B ≈ 85°~90° 时果断关火。
    需要说明越接近修正极限B的变化速度就越快，届时需要逐步降低推力调整
    > 若修正极限值仍旧过大（>1°），可环绕等待B降低为0然后重复本程序

【安全保护机制 (自动化)】
 ① 高倍速接近预警：当时间倍速 > 2x 且预测即将飞越目标天体时，系统会**自动暂停游戏**，防止错过捕获窗口。
 ② 变轨自动化停止：在霍曼转移加速中，若倍速 > 1 且预测 1 秒后速度将超过目标速度，系统会**自动暂停并降低倍速**至 1x，便于精确控制。

================================================================================
[3] 变轨黄金法则与捕获时机 (基于 Rp/Ra 及径向速度)
--------------------------------------------------------------------------------
法则 A (改变轨道形状，效率最高点) :
   * 在 远地点 (Ra) 加速 → 抬升 近地点 (Rp)  (改变轨道形状最省燃料)。
   * 在 近地点 (Rp) 减速 → 降低 远地点 (Ra)  (捕获入轨最省燃料)。

法则 B (坠入判据与捕获窗口) :
   * 坠入双重判据：只有在 **Ra 显示为负数** 且 **径向速度（接近速度）为负数** 时，
     才表示天体正在真正坠向中心。请用 `?` 查阅目标天体半径，确保 Rp 大于该半径。
   * 捕获最佳窗口：等待**径向速度大小达到最大值后开始减小并趋近 0 时**
     (即飞船即将到达近地点)，此时按法则 A 在近地点进行减速制动，捕获效率最高。
   * 注意：Rp 本身不会为负，若观测到 Rp 跨越 0 值，表示轨道方向发生了反向，
     此时在 `?` 面板中倾角显示会从 <90° 跳变至 >90°。

法则 C (微小径向调整) :
   * 若在到达目标天体近地点前需要微量调整近地点高度（确保不撞入目标天体），
     应使用 `wasd` 进行小幅度径向速度修正，而不是动用主引擎大油门。
     目标天体半径可在使用?选择天体时查看。若近地点处于安全高度，则建议接近
     近地点后再根据法则 AB 进行变轨机动入轨。

================================================================================
[4] 霍曼转移标准化执行检查单 (含自动化特性说明)
--------------------------------------------------------------------------------
前置条件 : 已按 f 锁定飞船，按 t 设定最终目标天体。
           **注意**：只需设 t，Rp/Ra 及霍曼数据即自动显示，无需按 F。

 [ ] 第一步 检查倾角 (共面性) ：
           观察 A(B)°/C°。若 A 值较大（>5°），应先按第2节方法修正倾角。
           非共面轨道盲目转移将直接导致错过目标。

 [ ] 第二步 观察 C 值与自动化提示：
           状态栏显示 `A(B)°/C°` 即代表共心。当 C 值趋近 0° 时准备点火。
           若当前油门 > 0 且 |C| < 10°，状态栏会显示“加速剩余时间: XX秒”。

 [ ] 第三步 启动点火与自动停止：
           按 Z 设定油门，按 空格 点火。
           系统会自动监控：若倍速 > 1 且预测 1 秒后速度超过所需霍曼速度，
           **系统将自动暂停并降倍速至 1x**，避免过度加速。

 [ ] 第四步 等待与安全暂停：
           若前往外行星，观察 Ra 抬升；若前往内行星，观察 Ra 降低。
           在目标接近过程中，若高倍速运行，**安全保护机制会自动暂停**防止飞越。

 [ ] 第五步 入轨捕获：
           接近目标时，参考法则 B 的捕获窗口，在近地点执行减速泊入。

================================================================================
【写在最后】 倾角修正如打靶，视线垂直黄灰夹；B 增倾减方向对，B 近九十即收兵。
            捕获要看径向速，峰值回零快减速；高倍暂停是保护，自动降速显神通。
            黄线是命根，灰线是老板，青线是KPI。
================================================================================
)";
	print_pager("QRH", sv_from_cstr(content), no_page?-2:0);
}

static bool input_handle(Runtimedata_t *rt)
{
	if (!rt) return false;
#define v_forward vec_direct(rt->active_cam->forward)
#define v_up      vec_direct(rt->active_cam->up)
#define v_right   vec_direct(vec_cross_product(rt->active_cam->forward, rt->active_cam->up))
	switch (rt->inp) {
	case '\t': rt_setup(rt, 1); break;
	case '~': rt_setup(rt, 0); break;
	case '`': rt_sync_cam_size_scale(rt); break;
	case '@': set_camera_forward(rt); break;
	case '#': rt->ignore_safety = !rt->ignore_safety; break;
	case '%': rt->hud_time_simple_format = !rt->hud_time_simple_format; break;
	case 'f':
		rt->follow = star_choose(rt, "跟随", rt->follow);
		if (!rt->follow) {
			rt_switch_camera(rt, rt->camera);
			break;
		}
		rt_switch_camera(rt, &rt->follow->cam);
		rt->about_point = star_get_about_point(rt, rt->follow);
		Vec_t direct = rt->about_point ? vec_sub(rt->follow->speed, rt->about_point->speed) : rt->follow->speed;
		if (vec_len(direct) == 0) direct = rt->active_cam->forward;
		direct = vec_direct(direct);
		const double distance = vec_len(vec_sub(rt->active_cam->position, rt->follow->center));
		rt->active_cam->position = vec_add(rt->follow->center, vec_mul(direct, -distance));
		camera_look_no_hold(rt->active_cam, rt->follow->center);
		break;
	case 't': rt->destination_to = star_choose(rt, "驶向", rt->destination_to); break;
	case 'F': rt->look_to = star_choose(rt, "看向", rt->look_to); break;
	case 'T': rt->rotate_cam_with_spd = !rt->rotate_cam_with_spd; break;
	case '!':
		rt->about_point = star_choose(rt, "固定为中心", rt->about_point);
		rt->fixed_about_point = rt->about_point;
		if (!rt->about_point) {
			rtlog(rt, "取消环绕中心强制固定");
			rt->about_point = star_get_about_point(rt, rt->follow);
		} else rtlog(rt, "强制切换固定天体环绕中心为'%s'", rt->about_point->name.p);
		break;
	case '$':
		rt->use_rk4 = !rt->use_rk4;
		rt->time_scale_limit = rt->use_rk4?4096:2048;
		break;
	case '"':
		print_pager("航行日志", sv_from_sva(&rt->logs), -1);
		rt->need_notification = 0;
		break;
	case '|':
		rt_dump_stars(rt, &rt->buff);
		printf("\e[0m\n\e[J%.*s（回车返回）\n", (int)rt->buff.len, rt->buff.p);
		kbhitGetchar();
		_getch();
		break;
	case '?':
		voyage_helper(rt, &rt->buff, NULL);
		printf("\e[0m\n\e[J%s（回车返回）\n", rt->buff.p);
		kbhitGetchar();
		_getch();
		break;
	case 'M': print_qrh(false); break;
	case '\'': rt->print_busy = !rt->print_busy; break;
	case 'i': rt->axis = !rt->axis; break;
	case 'I': rt->guidline = !rt->guidline; break;
	case '7': rt->active_cam->scale-=1; break;
	case '8': rt->active_cam->scale+=1; break;
	case '9': rt->active_cam->dept/=2; break;
	case '0': rt->active_cam->dept*=2; break;
	case '{': rt->time_scale=1.; break;
	case '}': rt->time_scale=(60.*60); break;
	case '[': rt->time_scale/=2; break;
	case ']': rt->time_scale*=2; break;
	case 'Z': rt->throttle=255; break;
	case 'X': rt->throttle=0; break;
	case 'z': rt->throttle+= rt->throttle<255?1:0; break;
	case 'x': rt->throttle-= rt->throttle>0?1:0; break;
	case 'r': rt->throttle_on^=0b10; break;
	case ' ':
		rt->throttle_on ^= 1;
		if (rt->throttle_on&1 && rt->time_scale >= 32) {
			rt->time_scale = 1.;
		}
		break;
	case '.':
		rt->pause = true;
		phy_update(rt);
		break;
	case 'P':
	case 'p':
		rt->pause = !rt->pause;
		/* 意外油门保护 */
		if (!rt->pause && rt->throttle_on&1 && rt->time_scale >= 32) {
			rt->pause = true;
			rt->throttle_on &= ~1;
		}
		break;
	case 'c':
		printf("\e[4l\e[2J");
		break;
	case 'q':
	case 'Q':
		return false;
		break;
#define cam_shift(vec, k) camera_shift(rt->active_cam, vec_mul(vec_direct(vec), (k)))
        case '-': cam_shift(v_forward, kunit2unit(-0.5)); break;
        case '=': cam_shift(v_forward, kunit2unit(0.5)); break;
	case '_': cam_shift(v_forward, rt->follow?-vec_len(vec_sub(rt->follow->center, rt->active_cam->position)):kunit2unit(-5e2)); break;
        case '+': cam_shift(v_forward, rt->follow?vec_len(vec_sub(rt->follow->center, rt->active_cam->position))/2:kunit2unit(5e2)); break;
	case 'W': cam_shift(v_up, kunit2unit(0.5)); break;
	case 'S': cam_shift(v_up, kunit2unit(-0.5)); break;
	case 'A': cam_shift(v_right, kunit2unit(-0.5)); break;
	case 'D': cam_shift(v_right, kunit2unit(0.5)); break;
#undef cam_shift
#define cam_rotate(vec, theta) \
		  rt->follow?\
		  camera_rotate_about_point(rt->active_cam, rt->follow->center, (vec), -(theta)):\
		  camera_rotate(rt->active_cam, (vec), (theta))
	case 'h': cam_rotate(v_up, M_PI/180);break;
	case 'j': cam_rotate(v_right, -M_PI/180);break;
	case 'k': cam_rotate(v_right, M_PI/180);break;
	case 'l': cam_rotate(v_up, -M_PI/180);break;
	case 'J': cam_rotate(v_forward, M_PI/180);break;
	case 'K': cam_rotate(v_forward, -M_PI/180);break;
	case 'H': cam_rotate(v_forward, M_PI/4);break;
	case 'L': cam_rotate(v_forward, -M_PI/4);break;
	case '<': cam_rotate(v_up, M_PI/4);break;
	case '>': cam_rotate(v_up, -M_PI/4);break;
#undef cam_rotate
	}
	if (!rt->follow) return true;

	/* 手动加速,并记录操作dv(禁止暂停加速) */
	const double accel = !rt->pause ? throttle2accel(rt->throttle)/rt->fps : 0;
#define accelerate(var, k) rt->follow->speed = vec_add(rt->follow->speed, vec_mul((var), (k))), rt->dv += accel
	switch (rt->inp) {
	case 'n': accelerate(v_forward, accel); break;
	case 'N': accelerate(v_forward, 10*accel); break;
	case 'b': accelerate(v_forward, -accel); break;
	case 'B': accelerate(v_forward, -10*accel); break;
	case 'w': accelerate(v_up, accel); break;
	case 's': accelerate(v_up, -accel); break;
	case 'a': accelerate(v_right, -accel); break;
	case 'd': accelerate(v_right, accel); break;
#undef accelerate
	}
#undef v_forward
#undef v_up
#undef v_right
	return true;
}

#define RAND01 ((double)rand()/RAND_MAX)
#define RAND12 (1+RAND01)
#define RAND_VEC(k) vec_mul((Vec_t){1-2*RAND01, 1-2*RAND01, 1-2*RAND01}, RAND12*(k))
#define RAND_COLOR ((Color_t){100+RAND01*155,100+RAND01*155,100+RAND01*155,-1})
#define l_star_create(name, mass, radius, r, v, u)	\
	do {								\
		double rand_num = 2*M_PI*RAND01;			\
		star = star_create(name, mass, radius,			\
				   vec_rotate((Vec_t){r,0,0}, u, rand_num),\
				   vec_rotate((Vec_t){0,v,0}, u, rand_num),\
				   NULL);			\
	} while(0)
static void scene_init_solar(Runtimedata_t *rt)
{
	if (!rt) return;
	Star_t star = {};
	Star_t *center = NULL;
	l_star_create("地球", 5.965e24, 6371, -149.6e6, -29.78, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(24*60*60);
	obj_rotate(obj_set_color(star.obj, (Color_t){29,153,243,-1}), (Vec_t){1, 1, -1}, M_PI/3.8);
	da_append(&rt->objs, &star);
	center = da_get(&rt->objs, rt->objs.len-1);

	star = star_create("地球小卫星", 1, 0.5, (Vec_t){12000,0,0}, vec_xyzl(0, 1, 0.8, 5.75993), center);
	obj_set_color(star.obj, (Color_t){-1,30,30,200});
	da_append(&rt->objs, &star);

	// GM = Rv^2
	// > sqrt((6.67*10^-11) * (5.965*10^24) / (11000*1000))/1000
	// 6.0141159707
	star = star_create("地球大卫星", 1e10, 450, (Vec_t){-42164,0,0}, vec_xyzl(0, -1, 0, 3.07282), center);
	obj_set_color(star.obj, (Color_t){0,-1,30,200});
	da_append(&rt->objs, &star);

	star = star_create("月球", 7.342e22, 1737.4,
			   vec_rotate((Vec_t){0, 384400, 0}, (Vec_t){1,0,0}, 5.14*M_PI/180),
			   vec_xyzl(-1, 0, 0, 1.022), center);
	obj_set_color(star.obj, (Color_t){100,100,100,-1});
	star.self_omiga = 2*M_PI/(30.5*24*60*60);
	da_append(&rt->objs, &star);

	l_star_create("水星", 3.301e23, 2439.7, 57.91e6, 47.87, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(1407.6*60*60);
	da_append(&rt->objs, &star);

	l_star_create("金星", 4.867e24, 6051.8, 108.21e6, 35.02, ((Vec_t){0,0,1}));
	star.self_rotate = (Vec_t){0, 0, -1};
	star.self_omiga = 2*M_PI/(5832.6*60*60);
	obj_set_color(star.obj, (Color_t){191,128,33,-1});
	da_append(&rt->objs, &star);

	l_star_create("火星", 6.417e23, 3389.5, 227.94e6, 24.07, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(24.6*60*60);
	obj_set_color(star.obj, (Color_t){227,124,93,-1});
	da_append(&rt->objs, &star);

	l_star_create("木星", 1.898e27, 69911, 778.57e6, 13.07, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(9.93*60*60);
	obj_set_color(star.obj, (Color_t){169,105,49,-1});
	da_append(&rt->objs, &star);

	l_star_create("土星", 5.683e26, 58232, 1433.53e6, 9.69, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(10.66*60*60);
	obj_set_color(star.obj, (Color_t){237,191,116,-1});
	da_append(&rt->objs, &star);

	l_star_create("天王星", 8.681e25, 25362, 2872.46e6, 6.81, ((Vec_t){0,0,1}));
	star.self_rotate = vec_rotate((Vec_t){0, 0, 1}, (Vec_t){0, -1, 0}, 97.77);
	star.self_omiga = 2*M_PI/(17.24*60*60);
	obj_set_color(star.obj, (Color_t){190,227,230,-1});
	da_append(&rt->objs, &star);

	l_star_create("海王星", 1.024e26, 24622, 4495.06e6, 5.43, ((Vec_t){0,0,1}));
	star.self_omiga = 2*M_PI/(16.11*60*60);
	obj_set_color(star.obj, (Color_t){45,55,140,-1});
	da_append(&rt->objs, &star);

	star = star_create("太阳", 1.989e30, 695700, (Vec_t){0,0,0}, (Vec_t){0,0,0}, NULL);
	star.self_omiga = 2*M_PI/(25.4*60*60);
	obj_set_color(star.obj, (Color_t){-1,-1,0,-1});
	da_append(&rt->objs, &star);
}
static void scene_init_three_body(Runtimedata_t *rt)
{
	if (!rt) return;
	Star_t star = {};
	Star_t *center = NULL;
	/* 安置在太阳系外4光年 */
	l_star_create("!?强强?!", 5.965e24*(10*RAND01+0.3), 6371*RAND12, 4*365*24*60*60*3e5, RAND12*5, ((Vec_t){0,0,1}));
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(24*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);
	center = da_get(&rt->objs, rt->objs.len-1);

	star = star_create("sun1", 1e31*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);

	star = star_create("sun2", 5e31*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);

	star = star_create("sun3", 1e32*(RAND12 - 0.5), RAND12*7e5, RAND_VEC(RAND12*1e8), RAND_VEC(5), center);
	star.self_rotate = vec_direct(RAND_VEC(1));
	star.self_omiga = 2*M_PI/(100*RAND12*60*60);
	obj_set_color(star.obj, RAND_COLOR);
	da_append(&rt->objs, &star);
	return;
}
#undef l_star_create
#undef RAND_COLOR
#undef RAND_VEC
#undef RAND12
#undef RAND01
static void scene_init(Runtimedata_t *rt, int add_three_body)
{
	if (!rt) return;
	if (add_three_body < 2) scene_init_solar(rt);
	if (add_three_body) scene_init_three_body(rt);
	star_sync_cam_config(rt);
}

static void rt_draw(Runtimedata_t *rt)
{
	if (rt->follow && rt->look_to) {
		Vec_t direct = rt->look_to == rt->follow ? \
			       vec_sub(rt->follow->speed, rt->about_point->speed) : \
			       vec_sub(rt->look_to->center, rt->follow->center);
		double dist = vec_len(vec_sub(rt->follow->center, rt->active_cam->position));
		rt->active_cam->position =
			vec_add(rt->follow->center,
				vec_mul(vec_direct(direct), -dist));
		camera_look_no_hold(rt->active_cam, rt->look_to->center);
	}

	if (rt->axis && rt->follow) {
		rt->axis_helper->center = rt->follow->center;
		obj_cast(rt->axis_helper, rt->active_cam, rt->backend);
	}
	if ((rt->guidline || rt->axis) && rt->follow) {
		Point_t p1, p2;
		camera_cast_line(rt->active_cam,
				 rt->follow->center,
				 vec_add(rt->follow->center,
					 vec_sub(rt->follow->speed, rt->about_point->speed)),
				 &p1, &p2);
		backend_draw_line(rt->backend, rt->active_cam, p1, p2,
				  (Color_t){-1,-1,0,UI_ALPHA},
				  (Color_t){-1,-1,0,UI_ALPHA});
	}
	if (rt->guidline && rt->follow && rt->destination_to) {
		Point_t p1, p2;
		/* 目的地方向 */
		camera_cast_line(rt->active_cam,
				 rt->follow->center,
				 rt->destination_to->center,
				 &p1, &p2);
		backend_draw_line(rt->backend, rt->active_cam, p1, p2,
				  (Color_t){0,-1,-1,UI_ALPHA},
				  (Color_t){0,-1,-1,UI_ALPHA});
		/* 当前环绕中心方向 */
		camera_cast_line(rt->active_cam,
				 rt->follow->center,
				 rt->about_point->center,
				 &p1, &p2);
		backend_draw_line(rt->backend, rt->active_cam, p1, p2,
				  (Color_t){-1,-1,-1,UI_ALPHA},
				  (Color_t){-1,-1,-1,UI_ALPHA});
	}
	for (size_t i = 0; i < rt->objs.len; i++) {
		Star_t *star = da_get(&rt->objs, i);
		if (!star->obj) continue;
		obj_cast(star->obj, rt->active_cam, rt->backend);
	}
	printf("\e[H");
	rt->backend->render(rt->backend);
	rt->backend->clean(rt->backend);
	if (!rt) return;
}

static void game_loop(Runtimedata_t *rt)
{
	if (!rt) return;
	size_t i = 0;
	double busy = 0;
	struct orbital_parameters ret = {};
	double last_e = 0, last2_e = 0;
	Star_t *last_about_point = NULL,
	       *last_follow = NULL;
	int8_t last_throttle_on = false;
	for (i = 0; i < INT64_MAX; ++i) {
		last_about_point = rt->about_point;
		last_follow = rt->follow;
		if ((rt->inp = kbhitGetchar()))
			if (!input_handle(rt)) break;
		if (rt->backend->get_input && (rt->inp = rt->backend->get_input(rt->backend)))
			if (!input_handle(rt)) break;
		if (rt->need_notification && !rt->ignore_safety) {
			rt->pause = true;
			print_pager("日志:发生事件", sv_from_sva(&rt->logs), -1);
			rt->need_notification = 0;
		}
		if (!rt->pause) phy_update(rt);
		if (rt->follow) ret = phy_get_orbital_parameters(rt->follow, rt->about_point);
		if (rt->follow && last_follow == rt->follow
		    && last_about_point && last_about_point->name.p
		    && last_about_point != rt->about_point) {
			format_orbital_parameters(rt, &rt->buff, ret);
			rtlog(rt, "天体'%s'被'%s'捕获(%s)(原运行在'%s'),累计dv:%.3gkm/s",
			      rt->follow->name.p, rt->about_point->name.p, rt->buff.p,
			      last_about_point->name.p, unit2km(rt->dv));
			rt->need_notification++;
		}
		if (!rt->about_point) break;
		const bool cond1 = rt->follow && last_follow == rt->follow && rt->about_point;
		const bool cond2 = cond1 ?    /* 变轨时近远地点高度交换(e变化方向变化) */
			(rt->throttle_on&1)
			&& (ret.e < 1 && last_e < 1)
			&& (last2_e > last_e && last_e < ret.e) : false;
		const bool cond3 = (last_throttle_on^rt->throttle_on)&1      /* 油门开关 */
			|| (rt->throttle_on&1 && (last_throttle_on^rt->throttle_on)&0b10);
		const bool cond4 = cond1 ? cond2    /* 触发以下任意事件 */
			|| cond3
			|| ((last_e-1)*(ret.e-1)<0) : false;    /* 轨道类型改变 */
		if (cond1 && cond4) {
			if (cond3) {
				rtlog(rt, "油门切换至: %s向推力 %d%% %s",
				      rt->throttle_on&0b10?"反":"正", rt->throttle, rt->throttle_on&1?"开":"关");
			}
			if (cond2) {
				rtlog(rt, "近远地点高度交换");
				if (rt->time_scale >= 2) rt->need_notification++;
			}
			format_orbital_parameters(rt, &rt->buff, ret);
			rtlog(rt, "%s[%s]:%s(%s),dv:%.3gkm/s",
			      rt->follow->name.p, rt->about_point->name.p,
			      ret.typ, rt->buff.p, unit2km(rt->dv));
		}
		last2_e = last_e;
		last_e = ret.e;
		last_throttle_on = rt->throttle_on;

		rt_draw(rt);
		int ret_code = 0;
		while ((ret_code = rt_format_huds(rt, &rt->buff, ret))) {
			if (ret_code & 1<<2) rt->pause = true;
			if (ret_code & 1<<1) rt->time_scale = 1;
			else if (ret_code & 1) rt->time_scale /= 2;
		}
		if (!rt->ignore_safety && rt->follow && rt->destination_to
		    && rt->time_scale > 32 && cond2) {
			rt->time_scale = 1;
			rt->pause = true;
		}
		/* 替换\n避免触发缓冲区刷新，减少屏闪 */
		sva_replace(&rt->buff, sv_from_lstr("\n"), sv_from_lstr("\r\e[B"));
		printf("\e[0m\r\e[J%s", rt->buff.p ? rt->buff.p : "");

		if (rt->pause) printf(" [已暂停]");
		if (rt->ignore_safety) printf(" <不安全>");
		if (rt->need_notification) printf(" [!%d]", rt->need_notification);
		if (rt->print_busy) printf(" (%5.1f%%/%dfps)", busy, (int)(busy<100?rt->fps:rt->fps*100/busy));
		busy = (busy + (1-(sleep_fixed_step(1./rt->fps))/(1./rt->fps)) * 100)/2;
	}
	return;
}

static void argv_help(char *argv)
{
	printf("Usage: %s\n"
	       "    -h        打印此信息\n"
	       "    -t        增设三体天体\n"
	       "    -i <FILE> 导入指定txt文件数据\n"
	       "    -o <FILE> 程序运行结束后自动导出日志数据的文件\n"
	       "    -s <SEED> 指定初始化种子\n"
	       "    -E <ARGS> 根据参数单次运行模拟并退出(需要配合-i)\n"
	       "    -p        初始化后导出数据并退出\n"
	       "    -M        打印内置操作手册\n",
	       argv);
}

int main(int argc, char *argv[])
{
	Runtimedata_t rt = {0};
	rt.seed = time(NULL);
	char *import_filename = NULL;
	char *output_filename = NULL;
	char *exec_cmd = NULL;
	bool init_only = false;
	int add_three_body = 0;
	while ((rt.inp=getopt(argc, argv, "hHtps:i:o:E:")) != -1) {
		switch (rt.inp) {
		case 'H':
			print_qrh(true);
			return 0;
			break;
		case 't':
			add_three_body++;
			break;
		case 'p':
			init_only = true;
			break;
		case 's':
			sscanf(optarg, "%u", &rt.seed);
			break;
		case 'i':
			import_filename = optarg;
			break;
		case 'o':
			output_filename = optarg;
			break;
		case 'E':
			exec_cmd = optarg;
			break;
		case 'h':
			argv_help(argv[0]);
			return 0;
			break;
		case '?':
			argv_help(argv[0]);
			return 1;
			break;
		}
	}
	if (!rt_setup(&rt, 0)) {
		return EXIT_FAILURE;
	}
	srand(rt.seed);
	const bool cond_default_init = !import_filename
		|| !rt_load_from_dumped_text(&rt, import_filename);
	if (cond_default_init) {
		if (import_filename)
			printf("[WARN] 加载'%s'失败，改用加载默认场景\n", import_filename);
		scene_init(&rt, add_three_body);
	}
	rt.camera->position = (Vec_t){0, 0, kunit2unit(1e6)};
	rt.camera->dept = kunit2unit(1e8);
	if (init_only) {
		rt_dump_stars(&rt, &rt.buff);
		printf("%.*s\n", (int)rt.buff.len, rt.buff.p);
		rt_cleanup(&rt);
		return EXIT_SUCCESS;
	}
	if (exec_cmd) {
		if (cond_default_init) {
			printf("[WARN] 应当加载存档再使用命令模式\n");
			rt_cleanup(&rt);
			return EXIT_FAILURE;
		}
		rt_exec_cmd(&rt, exec_cmd);
		FILE *fp;
		if (output_filename && (fp=fopen(output_filename, "a"))) {
			fprintf(fp,
				"[INFO] 保存时间:%ld\n"
				"==== 航行日志 ====\n%s%.*s\n",
				time(NULL), rt.logs.len ? rt.logs.p : "- 空 -\n",
				(int)rt.buff.len, rt.buff.p);
			fclose(fp);
		}
		rt_cleanup(&rt);
		return EXIT_SUCCESS;
	}
	rt.inp = 'f';
	input_handle(&rt);

	printf("\e[2J");
	star_sync_position(&rt);
	game_loop(&rt);

	while (rt.logs.p) {
		rt_dump_stars(&rt, &rt.buff);
		printf("\e[0m\n\e[J航行日志：\n%s%.*s", rt.logs.p, (int)rt.buff.len, rt.buff.p);
		if (!output_filename) break;
		FILE *fp = fopen(output_filename, "a");
		if (!fp) {
			fprintf(stderr, "[ERR] 无法打开输出文件'%s'\n", output_filename);
			break;
		}
		fprintf(fp,
			"[INFO] 保存时间:%ld\n"
			"==== 航行日志 ====\n%s%.*s\n",
			time(NULL), rt.logs.len ? rt.logs.p : "- 空 -\n",
			(int)rt.buff.len, rt.buff.p);
		fclose(fp);
		break;
	}
	rt_cleanup(&rt);
	return EXIT_SUCCESS;
}

