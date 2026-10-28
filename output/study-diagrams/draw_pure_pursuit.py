"""Exact, equal-scale geometry for the project's steering formula."""
from pathlib import Path
import math
import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Arc, Polygon
from matplotlib import font_manager
import numpy as np

OUT = Path(__file__).resolve().parent
font_manager.fontManager.addfont('C:/Windows/Fonts/malgun.ttf')
plt.rcParams.update({'font.family': 'Malgun Gothic', 'font.size': 16,
                     'axes.unicode_minus': False, 'mathtext.fontset': 'dejavusans'})
INK, MUTED, BLUE, TEAL, ORANGE, GRAY = '#1b2d40', '#526579', '#2564bb', '#087d79', '#b95112', '#cbd5df'
x, y, L = 5., 2., 2.8
R = (x*x+y*y)/(2*y)
delta = math.atan(L/R)

def setup(title, subtitle):
    fig = plt.figure(figsize=(12, 7.6), facecolor='white')
    fig.text(.045, .946, title, fontsize=24, color=INK, weight='bold')
    fig.text(.045, .893, subtitle, fontsize=15, color=MUTED)
    ax = fig.add_axes([.035, .13, .505, .70])
    ax.set_aspect('equal')
    ax.set_xlim(-1.45, 6.8)
    ax.set_ylim(-1.6, 8.4)
    ax.axis('off')
    tx = fig.add_axes([.575, .15, .40, .66])
    tx.axis('off')
    return fig, ax, tx

def line(ax, a, b, color=MUTED, lw=2, **kw):
    ax.plot([a[0],b[0]], [a[1],b[1]], color=color, lw=lw, **kw)

def arrow(ax, a, b, color=MUTED, both=False, lw=1.8):
    ax.annotate('', xy=b, xytext=a, arrowprops={'arrowstyle':'<->' if both else '-|>',
                'color':color, 'lw':lw, 'mutation_scale':15, 'shrinkA':0, 'shrinkB':0})

def text(ax, a, s, color=INK, size=16, **kw):
    return ax.text(*a, s, color=color, fontsize=size, **kw)

def point(ax, a, color=INK):
    ax.scatter([a[0]], [a[1]], s=48, c=color, zorder=8)

def note(tx, yy, s, size=17, color=INK, **kw):
    tx.text(0, yy, s, transform=tx.transAxes, fontsize=size, color=color, va='top', **kw)

fig, ax, tx = setup('1. 목표 좌표로 회전 반지름 R을 구한다',
                    '위에서 본 모습 · 차량은 오른쪽으로 진행 · 파란 원호는 뒷바퀴 중심의 경로')
O, C, P, D = (0,0), (0,R), (x,y), (0,y)
ax.add_patch(Polygon([C,D,P], facecolor=TEAL, alpha=.07, edgecolor='none'))
arrow(ax, (-.15,0), (6.35,0), GRAY)
arrow(ax, (0,-.15), (0,8.05), GRAY)
text(ax, (5.3,-.60), '+x 전방', size=15)
text(ax, (-.48,8.22), '+y 왼쪽', size=15)
line(ax,O,C,TEAL,2.3)
line(ax,C,P,TEAL,2.3)
line(ax,D,P,MUTED,1.7,ls=(0,(4,4)))
line(ax,P,(x,0),MUTED,1.5,ls=(0,(4,4)))
line(ax,(0,y+.25),(.25,y+.25),MUTED,1.4)
line(ax,(.25,y+.25),(.25,y),MUTED,1.4)
end = math.atan2(y-R,x)
t = np.linspace(-math.pi/2, end, 150)
ax.plot(R*np.cos(t), R+R*np.sin(t), color=BLUE, lw=4, zorder=5)
arrow(ax,(-.85,0),(-.85,R),TEAL,both=True)
text(ax,(-1.12,R/2),r'$R$',color=TEAL,size=23,ha='center')
text(ax,(.24,4.4),r'$R-y$',color=TEAL,size=22)
text(ax,(2.5,4.9),r'$R$',color=TEAL,size=23)
text(ax,(2.5,2.18),r'$x=5\,m$',size=20,ha='center',va='bottom')
arrow(ax,(5.45,0),(5.45,y),ORANGE,both=True)
text(ax,(5.66,.83),r'$y$',color=ORANGE,size=23)
text(ax,(.75,.72),'뒷바퀴의 원호',color=BLUE,size=15,rotation=10)
for p,c in [(O,INK),(C,TEAL),(P,ORANGE)]: point(ax,p,c)
text(ax,(-.15,-.55),'O (0, 0)',size=17,ha='center')
text(ax,(-.2,-1.12),'뒷바퀴 중심',size=14,ha='center',color=MUTED)
text(ax,(.28,7.38),'C (0, R)  회전 중심',size=16,color=TEAL)
text(ax,(5.15,2.57),'P (x, y)',size=19,color=ORANGE)
text(ax,(5.15,3.04),'목표점',size=15,color=ORANGE)
note(tx,.98,'두 반지름은 같다: CO = CP = R',size=17)
note(tx,.86,'직각삼각형에 피타고라스를 적용',size=16,color=MUTED)
note(tx,.75,r'$x^2+(R-y)^2=R^2$',size=23)
note(tx,.60,'괄호를 전개하고 정리하면',size=16,color=MUTED)
note(tx,.49,r'$x^2+y^2=2Ry$',size=24)
note(tx,.32,r'$R=\dfrac{x^2+y^2}{2y}$',size=29,color=TEAL)
note(tx,.08,'그림의 예: x = 5m, y = 2m',size=16)
note(tx,-.02,'R = 29 / 4 = 7.25m',size=19,color=TEAL)
fig.text(.045,.045,'왼쪽 목표(y > 0)의 예시. 원호는 현재 진행 방향에 접하고, 목표점을 지난다.',fontsize=14,color=MUTED)
fig.savefig(OUT/'01-target-to-radius.png',dpi=170,facecolor='white')
plt.close(fig)

fig, ax, tx = setup('2. 회전 반지름 R을 앞바퀴 조향각 δ로 바꾼다',
                    '같은 회전 중심 C · 앞·뒤 바퀴를 하나씩으로 표현한 자전거 모델 · 미끄러짐 없음')
F = (L,0)
ax.add_patch(Polygon([O,C,F],facecolor=BLUE,alpha=.055,edgecolor='none'))
line(ax,O,C,TEAL,2.3)
line(ax,C,F,MUTED,1.8,ls=(0,(5,4)))
line(ax,O,F,INK,3.1)
line(ax,F,(5.65,0),GRAY,1.8,ls=(0,(4,3)))
line(ax,(0,.27),(.27,.27),MUTED,1.4)
line(ax,(.27,.27),(.27,0),MUTED,1.4)
direction=np.array([math.cos(delta),math.sin(delta)])
normal=np.array([-math.sin(delta),math.cos(delta)])
fv=np.array(F)
# Equivalent rear wheel and steered front wheel, both centered at their axles.
line(ax,(-.53,0),(.53,0),INK,9,solid_capstyle='round')
line(ax,tuple(fv-.55*direction),tuple(fv+.55*direction),ORANGE,9,solid_capstyle='round')
arrow(ax,tuple(fv+.55*direction),tuple(fv+2.55*direction),ORANGE,lw=2.3)
# Radius to front axle is perpendicular to the front wheel's rolling direction.
q=.30
corners=[fv+q*normal, fv+q*(normal+direction), fv+q*direction]
ax.plot([p[0] for p in corners],[p[1] for p in corners],color=MUTED,lw=1.4)
ax.add_patch(Arc(F,3.1,3.1,theta1=0,theta2=math.degrees(delta),color=ORANGE,lw=2.2))
text(ax,(4.63,.13),r'$\delta$',size=25,color=ORANGE)
text(ax,(4.25,1.45),'앞바퀴 진행 방향',size=14,color=ORANGE,ha='center')
text(ax,(4.72,-.5),'차체 전방',size=14,color=MUTED,ha='center')
ax.add_patch(Arc(C,2.4,2.4,theta1=-90,theta2=-90+math.degrees(delta),color=ORANGE,lw=2))
text(ax,(.12,5.45),r'$\delta$',size=24,color=ORANGE)
text(ax,(-.45,3.55),r'$R$',size=24,color=TEAL)
arrow(ax,(0,-1.06),(L,-1.06),BLUE,both=True)
text(ax,(L/2,-1.47),r'$L=2.8\,m$',size=20,color=BLUE,ha='center')
point(ax,C,TEAL)
text(ax,(.25,7.34),'C  회전 중심',size=17,color=TEAL)
text(ax,(-.1,-.60),'O 뒷바퀴',size=14,ha='center')
text(ax,(L,-.60),'F 앞바퀴',size=14,ha='center')
note(tx,.98,'L: 앞·뒤 차축 사이 거리',size=18)
note(tx,.86,'삼각형 COF에서 같은 각 δ를 보면',size=16,color=MUTED)
note(tx,.75,r'$\tan\delta=\dfrac{L}{R}$',size=29)
note(tx,.56,r'$\delta=\operatorname{atan}\!\left(\dfrac{L}{R}\right)$',size=26)
note(tx,.35,'앞에서 구한 R을 대입',size=16,color=MUTED)
note(tx,.23,r'$\delta=\operatorname{atan}\!\left(\dfrac{2Ly}{x^2+y^2}\right)$',size=26,color=ORANGE)
note(tx,-.01,f'그림의 예: δ = +{math.degrees(delta):.2f}°',size=19,color=ORANGE)
fig.text(.045,.045,'δ는 차체 전방과 앞바퀴 방향 사이의 각도. 위 삼각형의 δ와는 두 변이 각각 수직이므로 같다.',fontsize=14,color=MUTED)
fig.savefig(OUT/'02-radius-to-steering.png',dpi=170,facecolor='white')
plt.close(fig)

assert abs(x*x+(y-R)**2-R**2) < 1e-10
assert abs(np.dot(np.array(C)-fv,direction)) < 1e-10
assert abs(math.atan2(L,R)-delta) < 1e-10
assert abs((R*math.cos(end))-x) < 1e-10
assert abs((R+R*math.sin(end))-y) < 1e-10
checks={'illustrative_example':{'x_m':x,'y_m':y,'L_m':L,'R_m':R,
                              'steering_deg':math.degrees(delta)},
        'project_example':{'x_m':5.15,'y_m':-.97,'L_m':2.8,
                           'steering_deg':math.degrees(math.atan(2*2.8*(-.97)/(5.15**2+.97**2)))},
        'geometry_checks':'circle equation, target endpoint, perpendicular wheel, angle equality passed'}
(OUT/'geometry-checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(checks,ensure_ascii=False))
