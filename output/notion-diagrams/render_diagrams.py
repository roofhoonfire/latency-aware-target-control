"""Deterministic study diagrams. Dimensions, bit order and values are explicit."""
from pathlib import Path
import json
import math
import struct
import binascii
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
W = 1600
INK = '#192D40'
MUTED = '#506479'
BLUE = '#245DB5'
TEAL = '#007D79'
ORANGE = '#AE5C0A'
LINE = '#CFDAE4'
PALE = '#EFF4F9'
BP = '#EAF2FF'
TP = '#E8F6F2'
OP = '#FFF2DF'
WHITE = '#FFFFFF'
FONT = 'C:/Windows/Fonts/malgun.ttf'
BOLD = 'C:/Windows/Fonts/malgunbd.ttf'
MONO = 'C:/Windows/Fonts/consola.ttf'
manifest = []
text_boxes = []

def f(size=32, bold=False, mono=False):
    return ImageFont.truetype(MONO if mono else BOLD if bold else FONT, size)

def label(x, y, s, size=32, color=INK, bold=False, align='left', mono=False):
    font = f(size, bold, mono)
    for i, line in enumerate(s.split('\n')):
        width = d.textlength(line, font=font)
        xx = x - width/2 if align == 'center' else x-width if align == 'right' else x
        yy = y+i*int(size*1.5)
        box = d.textbbox((xx, yy), line, font=font)
        assert box[0] >= 18 and box[2] <= W-18, (name, line, box)
        assert box[1] >= 0 and box[3] <= im.height-12, (name, line, box)
        text_boxes.append((name, line, box))
        d.text((xx, yy), line, fill=color, font=font)

def rect(x, y, w, h, fill=PALE, outline=None, radius=20):
    d.rounded_rectangle((x,y,x+w,y+h), radius=radius, fill=fill, outline=outline, width=2)

def rule(y): d.line((70,y,1530,y), fill=LINE, width=2)

def arrow(x1,y1,x2,y2,color=MUTED,width=4,head=13):
    d.line((x1,y1,x2,y2),fill=color,width=width)
    a=math.atan2(y2-y1,x2-x1)
    d.polygon([(x2,y2),(x2-head*math.cos(a-.55),y2-head*math.sin(a-.55)),(x2-head*math.cos(a+.55),y2-head*math.sin(a+.55))], fill=color)

def dimension(x1,x2,y,title,color=BLUE):
    arrow(x1,y,x2,y,color,3,10)
    arrow(x2,y,x1,y,color,3,10)
    label((x1+x2)/2,y+12,title,30,color,bold=True,align='center')

def box(x,y,w,h,title,body='',fill=PALE,accent=BLUE,title_size=36,body_size=30):
    rect(x,y,w,h,fill)
    d.rounded_rectangle((x,y,x+7,y+h), radius=3, fill=accent)
    label(x+28,y+17,title,title_size,accent,True)
    if body: label(x+28,y+72,body,body_size)

def begin(filename,title,subtitle,height,number):
    global im,d,name
    name=filename
    im=Image.new('RGB',(W,height),WHITE)
    d=ImageDraw.Draw(im)
    label(70,36,f'LATC  /  {number:02d}',25,TEAL,True)
    label(70,87,title,52,INK,True)
    label(70,167,subtitle,29,MUTED)
    rule(230)

def finish(block,caption):
    rule(im.height-82)
    label(70,im.height-65,'이기종 제어 프로젝트 · 현재 소스와 본문 예시 기준',24,MUTED)
    label(1530,im.height-65,'STM32 / Zynq',24,MUTED,align='right')
    path=OUT/name
    im.save(path,optimize=True)
    manifest.append({'block_id':block,'filename':name,'path':str(path),'caption':caption,'width':W,'height':im.height})

# Numeric evidence used by every diagram.
angle=math.degrees(math.atan(2*2.8*(-.97)/(5.15**2+.97**2)))
pulse=round(1500+(-angle/30)*(1200-1500))
payload=struct.pack('<HhhH',1,515,-97,0)
crc=binascii.crc_hqx(bytes([1,1,8])+payload,0xffff)
frame=bytes([0xaa,0x55,1,1,8])+payload+struct.pack('<H',crc)
assert pulse==1388 and crc==0x5a5e
assert frame.hex(' ').upper()=='AA 55 01 01 08 01 00 03 02 9F FF 00 00 5E 5A'
assert len(frame)==15 and len(payload)==8
assert 20000*1.25==25000 and pulse*1.25==1735

# 01: single downward path; final platform outputs branch, never run in series.
begin('01-end-to-end.png','검출 한 건이 PWM 명령까지 가는 길','위에서 아래로 읽기 · 수신은 도착할 때, 제어는 명목 20ms마다 실행',2030,1)
box(140,275,1320,150,'① Simulink: 검출 한 건 선택','t, x, y, vx, vy를 double 5개로 포장 → UDP 데이터 40바이트',BP)
arrow(800,427,800,466)
box(140,470,1320,150,'② Linux: 제어용 명령 생성','x=515cm, y=-97cm, seq=1, prediction_ms=0 → 직렬화·헤더·CRC',BP)
arrow(800,622,800,661)
box(140,665,1320,150,'③ UART 선로: 115200 · 8N1','응용 프레임 15바이트 → 문자마다 시작·정지 비트 → 총 150비트',OP,ORANGE)
arrow(800,817,800,856)
box(140,860,1320,150,'④ 보드 UART → ISR → 바이트 큐','UART가 데이터 바이트를 복원 · ISR이 바이트와 epoch를 큐에 전달',PALE)
arrow(800,1012,800,1051)
box(140,1055,1320,150,'⑤ 수신 태스크 → 목표 큐','프레임 검증·TargetCommand 복원 → 좌표만 TargetState {515, -97}',PALE)
arrow(800,1207,800,1246)
box(140,1250,1320,195,'⑥ 제어 태스크가 공통 함수를 호출','공통 제어기: 유효 목표·timeout 판단 → 약 -11.188°\n공통 SG90 매퍼: 각도 → PwmCommand {20000μs, 1388μs}',TP,TEAL)
d.line((800,1447,800,1505),fill=MUTED,width=4)
d.line((435,1505,1165,1505),fill=MUTED,width=4)
arrow(435,1505,435,1550)
arrow(1165,1505,1165,1550)
box(110,1555,650,230,'STM32: 물리 PWM 구현','TIM4 / 채널 2 / PB7\nARR = 24999, CCR2 = 1735\n핀에서 반복 파형 출력',BP,BLUE,34,30)
box(840,1555,650,230,'Zynq: 계산 결과 기록','같은 공통 제어기·매퍼 실행\n같은 PwmCommand 기록\n물리 PWM 백엔드 구현 없음',OP,ORANGE,34,30)
label(800,1840,'화살표는 데이터 흐름이다. 수신 한 건마다 제어 태스크가 즉시 실행되는 것은 아니다.',27,MUTED,align='center')
finish(0,'전체 데이터 흐름. STM32는 PWM을 출력하고, 현재 Zynq 구현은 공통 계산 결과를 기록한다.')

# 02: functions are labels; this replaces the old call-tree picture, not algorithms.
begin('02-linux-encoding.png','Linux에서 15바이트를 만드는 순서','transmit_detection() 안에서 수행 · 각 단계가 성공한 정상 경로',1270,2)
box(190,275,1220,150,'detection_to_target_command()','검출 좌표 m → cm · sequence 설정 · prediction_ms = 0',BP)
arrow(800,427,800,476)
rect(145,480,1310,440,TP)
label(180,500,'encode_target_command_frame()',34,TEAL,True)
box(190,570,1220,130,'target_command_serialize()','TargetCommand의 네 필드 → 리틀엔디언 페이로드 8바이트',WHITE,TEAL,35,29)
arrow(800,702,800,746,TEAL)
box(190,750,1220,130,'uart_frame_encode()','헤더 5 + 페이로드 8 + CRC 2 → 응용 프레임 15바이트',WHITE,TEAL,35,29)
arrow(800,922,800,971)
box(190,975,1220,150,'SerialPort::write_all()','15바이트 전체를 시리얼 장치에 전달 · 비트 신호는 UART가 생성',OP,ORANGE)
finish(8,'Linux 송신 호출 흐름. 8바이트 직렬화와 15바이트 프레임 포장은 서로 다른 단계다.')

# 03: proportional frame overview plus expanded payload, offsets explicitly shown.
begin('03-frame-layout.png','15바이트 프레임 안에 8바이트 페이로드','1바이트 = 8비트 · 아래 너비는 바이트 수에 비례',1030,3)
x0=80; cell=96; y=330
for start,n,title,fill,color in [(0,2,'SOF',PALE,INK),(2,1,'ver',BP,BLUE),(3,1,'type',BP,BLUE),(4,1,'len',BP,BLUE),(5,8,'payload',TP,TEAL),(13,2,'CRC',OP,ORANGE)]:
    x=x0+start*cell
    d.rectangle((x,y,x+n*cell-4,y+130),fill=fill)
    label(x+n*cell/2-2,y+15,title,28,color,True,align='center')
    label(x+n*cell/2-2,y+66,f'{n}B',35,color,True,align='center')
for i in range(15): label(x0+cell*(i+.5),280,str(i),25,MUTED,align='center')
label(80,245,'바이트 위치 (0부터)',25,MUTED)
dimension(80,560,485,'헤더 5바이트',BLUE)
dimension(560,1328,485,'페이로드 8바이트',TEAL)
dimension(1328,1520,485,'CRC 2바이트',ORANGE)
label(80,590,'payload만 확대: 모든 필드는 16비트 = 2바이트',36,TEAL,True)
fields=[('sequence','uint16_t'),('target_x','int16_t'),('target_y','int16_t'),('prediction_ms','uint16_t')]
for i,(field,typ) in enumerate(fields):
    x=80+i*360
    rect(x,660,346,160,TP)
    label(x+173,677,field,30,TEAL,True,align='center')
    label(x+173,728,f'{typ} · 2B',30,INK,align='center')
label(800,865,'이 8바이트는 통신 규약의 직렬화 크기다. C 구조체를 그대로 복사해 보내는 방식이 아니다.',28,MUTED,align='center')
finish(9,'UART 응용 프레임 15바이트 = 헤더 5 + 페이로드 8 + CRC 2. 페이로드의 네 필드는 각각 16비트다.')

# 04: exact byte order and CRC range, generated from independently calculated bytes.
begin('04-frame-bytes.png','예제 한 건의 실제 15바이트','seq=1 · x=515cm · y=-97cm · prediction_ms=0 · 16진수 표기',1160,4)
for i,b in enumerate(frame):
    x=80+i*96
    fill=TP if 5<=i<13 else OP if i>=13 else BP
    label(x+46,268,str(i),25,MUTED,align='center')
    rect(x,313,92,92,fill,radius=9)
    label(x+46,330,f'{b:02X}',41,INK,True,align='center',mono=True)
d.rectangle((80+2*96,430,80+13*96-4,440),fill=TEAL)
label(80+(2+13)*96/2,463,'CRC 계산 범위: version · type · length · payload (11바이트)',30,TEAL,True,align='center')
label(80,535,'헤더: AA 55 = SOF · 01 = version · 01 = type · 08 = payload 길이',30,INK)
cards=[('01 00 → sequence = 1','uint16 · 하위 바이트부터 전송'),('03 02 → target_x = 515','0x0203 = 515cm'),('9F FF → target_y = -97','int16 · 0xFF9F를 부호 있는 값으로 해석'),('00 00 → prediction_ms = 0','현재 baseline은 예측 보정 시간을 0으로 설정')]
for i,(title,body) in enumerate(cards):
    x=80+(i%2)*740; yy=620+(i//2)*160
    box(x,yy,700,130,title,body,TP,TEAL,32,26)
box(80,950,1440,100,'CRC = 0x5A5E → 5E 5A 순서로 전송',fill=OP,accent=ORANGE,title_size=35)
finish(10,'검산한 예제 프레임. CRC는 SOF와 CRC 자체를 제외한 11바이트를 계산하며, 결과 0x5A5E를 5E 5A로 저장한다.')

# 05: equal-width bit cells; electrical levels correspond exactly to 0xAA LSB first.
begin('05-uart-8n1.png','8N1: 데이터 1바이트를 10비트로 전송','예: 0xAA · 데이터는 최하위 비트 b0부터 · 패리티 비트 없음',1160,5)
bits=[0]+[(0xaa>>i)&1 for i in range(8)]+[1]
start=160; cw=128
for i,v in enumerate(bits):
    x=start+i*cw
    fill=TP if i in (0,9) else BP
    label(x+cw/2,280,'start' if i==0 else 'stop' if i==9 else f'b{i-1}',29,TEAL if i in (0,9) else BLUE,True,align='center')
    rect(x+2,335,cw-4,90,fill,radius=10)
    label(x+cw/2,345,str(v),46,INK,True,align='center')
hi,lo=485,590
label(70,478,'1',28,MUTED)
label(70,580,'0',28,MUTED)
d.line((105,hi,start,hi),fill=TEAL,width=5)
prev=hi
for i,v in enumerate(bits):
    x=start+i*cw; yy=hi if v else lo
    d.line((x,prev,x,yy,x+cw,yy),fill=BLUE,width=5)
    d.line((x,450,x,625),fill=LINE,width=1)
    prev=yy
d.line((start+10*cw,prev,1510,hi),fill=TEAL,width=5)
label(70,640,'idle=1',26,TEAL)
arrow(230,662,1490,662)
label(850,678,'時間 순서: start → b0 … b7 → stop'.replace('時間','시간'),29,MUTED,align='center')
box(80,760,700,145,'데이터 8 + start 1 + stop 1','한 문자 = 10비트 · 8N1 설정 기준',BP,BLUE,34,30)
box(820,760,700,145,'15바이트 × 10 = 150비트','데이터 120비트 + 시작·정지 30비트',TP,TEAL,34,30)
label(800,955,'115200 bit/s에서 약 1.302ms · 문자 사이 추가 공백이 없을 때의 명목 전송 시간',29,INK,align='center')
label(800,1007,'보드 소프트웨어는 10비트 묶음이 아니라 UART가 복원한 8비트 데이터 바이트를 읽는다.',28,MUTED,align='center')
finish(11,'0xAA의 실제 비트 순서와 8N1 구성. 응용 프레임 15바이트는 문자 15개, 선로에서는 시작·정지 비트를 포함해 150비트다.')

# 06: calculation card, deliberately not a steering-geometry sketch with invented axes.
begin('06-steering-calculation.png','좌표에서 약 -11.188°가 나오는 과정','유효한 목표가 있고, 마지막 반영 후 400ms 미만인 경우',1030,6)
for x,title,value in [(80,'목표 x','5.15m'),(570,'목표 y','-0.97m'),(1060,'설정 L','2.8m')]:
    box(x,275,460,160,title,value,BP,BLUE,32,43)
arrow(800,438,800,483)
rect(120,490,1360,210,TP)
label(800,515,'δ = atan(2 × L × y / (x² + y²)) × 180 / π',42,TEAL,True,align='center')
label(800,593,'= atan(-5.432 / 27.4634) × 180 / π → 약 -11.188°',36,INK,align='center')
arrow(800,703,800,748)
box(120,755,1360,145,'절댓값 |δ|는 약 11.188° · 1°보다 크고 30°보다 작음','deadband에도, ±30° 제한에도 걸리지 않음 → 약 -11.188° 유지',TP,TEAL,36,32)
finish(19,'공통 제어기의 좌표 계산 예시. deadband와 제한은 각도의 절댓값으로 판단하며, 이 입력에서는 계산 결과가 유지된다.')

# 07: mapping chart with explicit calibrated points and a rounded pulse example.
begin('07-angle-to-pulse.png','SG90 매퍼: 각도 → HIGH 펄스 폭','현재 프로젝트의 임시 보정 설정 · 실제 서보 각도 실측값을 뜻하지 않음',1150,7)
xl,xr,yt,yb=230,1400,325,760
def xy(a,p): return xl+(a+30)/60*(xr-xl),yb-(p-1200)/600*(yb-yt)
label(80,260,'HIGH 폭 (μs)',29,MUTED)
for p in [1200,1500,1800]:
    yy=xy(0,p)[1]
    d.line((xl,yy,xr,yy),fill=LINE,width=2)
    label(195,yy-22,str(p),29,MUTED,align='right')
arrow(xl,yb+25,xl,yt-20,MUTED,3)
arrow(xl-20,yb,xr+30,yb,MUTED,3)
pts=[xy(-30,1200),xy(0,1500),xy(30,1800)]
d.line(pts,fill=TEAL,width=7)
for a,p in [(-30,1200),(0,1500),(30,1800)]:
    x,y=xy(a,p)
    d.ellipse((x-9,y-9,x+9,y+9),fill=TEAL)
    label(x,yb+35,f'{a:+d}°' if a else '0°',30,MUTED,align='center')
x,y=xy(angle,pulse)
d.line((x,y,x,yb),fill=ORANGE,width=3)
d.ellipse((x-12,y-12,x+12,y+12),fill=ORANGE)
rect(770,620,650,96,OP)
label(1095,640,'-11.188° → 반올림 후 1388μs',31,ORANGE,True,align='center')
arrow(770,668,x+16,y+2,ORANGE,3)
label(1400,840,'입력 조향각 (°)',29,MUTED,align='right')
box(120,915,1360,130,'반복 주기는 고정: 20000μs = 20ms = 50Hz','매퍼는 시간을 μs 단위로 출력한다. 타이머 레지스터 값은 아직 계산하지 않는다.',BP,BLUE,34,28)
finish(20,'현재 매퍼 설정의 각도와 HIGH 폭 관계. -30°는 1200μs, 0°는 1500μs, +30°는 1800μs이며 반복 주기는 20ms다.')

# 08: frequency conversion first, then two independent register calculations.
begin('08-timer-counts.png','STM32: μs를 타이머 count로 바꾸기','TIM4 · 상향 카운트 · PWM 모드 1 · 채널 2',1220,8)
for x,w,title,body,fill,col in [(80,430,'TIM4 입력 클록','6.25MHz',BP,BLUE),(590,400,'PSC = 4','PSC + 1 = 5로 나눔',PALE,INK),(1070,450,'카운터 속도','1.25MHz',TP,TEAL)]:
    box(x,275,w,160,title,body,fill,col,34,32)
arrow(515,355,583,355)
arrow(995,355,1063,355)
label(800,470,'1 count = 1 / 1.25MHz = 0.8μs',40,TEAL,True,align='center')
rule(555)
rows=[(610,'반복 주기','20000μs','25000 counts','ARR = 24999',BLUE,BP),(835,'HIGH 폭','1388μs','1735 counts','CCR2 = 1735',TEAL,TP)]
for yy,kind,us,counts,reg,col,fill in rows:
    label(80,yy-12,kind,31,col,True)
    box(80,yy+45,430,120,us,fill=fill,accent=col,title_size=40)
    label(550,yy+12,'÷ 0.8μs',25,MUTED,align='center')
    arrow(515,yy+103,585,yy+103)
    box(590,yy+45,400,120,counts,fill=fill,accent=col,title_size=35)
    label(1030,yy+12,'- 1' if kind=='반복 주기' else '그대로',25,MUTED,align='center')
    arrow(995,yy+103,1063,yy+103)
    box(1070,yy+45,450,120,reg,fill=fill,accent=col,title_size=36)
label(800,1080,'CNT가 0부터 ARR까지 세므로, 한 주기의 count 수는 ARR + 1이다.',30,INK,align='center')
finish(23,'TIM4 입력 6.25MHz를 PSC+1=5로 나누어 1.25MHz로 센다. 주기에는 ARR+1 관계를, HIGH 폭에는 CCR2를 적용한다.')

# 09: waveform drawn to scale, with exact leading-edge and trailing-edge markers.
begin('09-pwm-waveform.png','PB7 / TIM4_CH2: 계산상 예상 PWM 파형','실측 캡처가 아님 · 20ms 주기 · HIGH 1.388ms · 시간축은 비례',1130,9)
left,right=200,1470
scale=(right-left)/22
def tx(ms): return left+ms*scale
hi,lo=440,670
for t in [0,5,10,15,20]:
    x=tx(t)
    d.line((x,350,x,730),fill=LINE,width=2)
    label(x,745,str(t),28,MUTED,align='center')
for start_ms in [0,20]:
    d.rectangle((tx(start_ms),hi,tx(start_ms+1.388),lo),fill=TP)
path=[(left,lo),(left,hi),(tx(1.388),hi),(tx(1.388),lo),(tx(20),lo),(tx(20),hi),(tx(21.388),hi),(tx(21.388),lo),(right,lo)]
d.line(path,fill=TEAL,width=7)
label(80,hi-25,'HIGH',29,TEAL,True)
label(80,lo-25,'LOW',29,MUTED,True)
label(1470,790,'시간 (ms)',28,MUTED,align='right')
label(200,275,'HIGH = 1388μs = 1.388ms',35,TEAL,True)
arrow(360,333,tx(.694),420,TEAL,3)
dimension(tx(0),tx(20),840,'반복 주기 = 20000μs = 20ms',BLUE)
label(800,935,'HIGH: CNT < CCR2(1735) · 한 주기: CNT = 0 … ARR(24999)',31,INK,align='center')
label(800,992,'명목 duty = 1388 / 20000 = 6.94% · 다음 주기에도 하드웨어가 파형을 반복한다.',29,MUTED,align='center')
finish(26,'계산상 예상 파형이며 실측이 아니다. HIGH 폭 1.388ms와 반복 주기 20ms를 시간축 비율에 맞추어 표시했다.')

# 10: Zynq explicitly ends at recorded values, no pin icon / waveform implied.
begin('10-zynq-boundary.png','Zynq에서 현재 구현이 끝나는 지점','같은 공통 제어기와 SG90 매퍼를 실행 · 물리 PWM 백엔드는 구현되어 있지 않음',1110,10)
box(180,275,1240,150,'공통 제어기','TargetState {515, -97} → angle은 약 -11.188°',TP,TEAL,36,36)
arrow(800,427,800,476,TEAL)
box(180,480,1240,190,'공통 SG90 매퍼','PwmCommand.period_us = 20000\nPwmCommand.pulse_width_us = 1388',TP,TEAL,36,34)
arrow(800,672,800,721,TEAL)
box(180,725,1240,130,'trace 등 기록 경로에 계산 결과 반영',fill=BP,accent=BLUE,title_size=37)
d.line((360,915,1240,915),fill=ORANGE,width=5)
label(800,942,'현재 구현 경계: 여기서 PWM 핀 출력으로 이어지지 않는다.',32,ORANGE,True,align='center')
finish(27,'Zynq의 구현 범위. 공통 제어기와 매퍼의 결과를 기록하지만 물리 PWM 핀 출력 단계는 없다.')

# 11: number journey, parallel final branches and transport/semantic boundaries.
begin('11-value-journey.png','한 건을 숫자로 다시 따라가기','예제: seq=1 · x=5.15m · y=-0.97m · prediction_ms=0',1810,11)
stages=[
    ('① 검출 좌표','5.15m, -0.97m',BP,BLUE),
    ('② 단위 변환','515cm, -97cm',BP,BLUE),
    ('③ 명령 직렬화','TargetCommand → 페이로드 8바이트',TP,TEAL),
    ('④ 프레임 포장·UART 전송','응용 프레임 15바이트 → 8N1 문자 15개 → 150비트',OP,ORANGE),
    ('⑤ 수신·검증·역직렬화','TargetCommand 복원 → TargetState {515, -97}',PALE,BLUE),
    ('⑥ 제어기·매퍼','약 -11.188° → 주기 20000μs / HIGH 폭 1388μs',TP,TEAL),
]
for i,(title,body,fill,col) in enumerate(stages):
    yy=275+i*181
    box(140,yy,1320,140,title,body,fill,col,32,32)
    if i<5: arrow(800,yy+142,800,yy+176)
d.line((800,1322,800,1380),fill=MUTED,width=4)
d.line((440,1380,1160,1380),fill=MUTED,width=4)
arrow(440,1380,440,1425)
arrow(1160,1380,1160,1425)
box(110,1430,660,210,'STM32','ARR = 24999, CCR2 = 1735\nTIM4 / CH2 / PB7에 적용',BP,BLUE,38,32)
box(830,1430,660,210,'Zynq','동일한 PwmCommand 기록\n물리 PWM 출력 구현 없음',OP,ORANGE,38,32)
label(800,1670,'8바이트·15바이트·150비트는 서로 다른 계층의 크기다.',31,INK,True,align='center')
finish(28,'동일 예제의 값 변화. 페이로드 8바이트, 응용 프레임 15바이트, 선로의 150비트를 구분하고 플랫폼별 마지막 단계를 나눴다.')

(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'numeric-checks.json').write_text(json.dumps({'angle_deg':angle,'pulse_us':pulse,'frame_hex':frame.hex(' ').upper(),'crc_hex':hex(crc),'period_counts':25000,'ARR':24999,'CCR2':1735,'uart_bits':150,'uart_time_ms':150/115200*1000,'checked_text_boxes':len(text_boxes)},indent=2),encoding='utf-8')
print(json.dumps({'created':len(manifest),'paths':[m['filename'] for m in manifest],'text_bounds_checked':len(text_boxes)}))
