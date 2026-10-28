from pathlib import Path
from io import BytesIO
from pypdf import PdfReader,PdfWriter
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
import pypdfium2 as pdfium
ROOT=Path(r'C:/Github/latency-aware-target-control')
FONT=Path(r'C:/Users/threh/.codex/visualizations/2026/10/19/01a153e6-4943-76d0-ba95-a7628937c9be/lig-slide/assets/pretendard')
for name,file in [('PR','Pretendard-Regular.ttf'),('PB','Pretendard-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,str(FONT/file)))
SRC=Path(r'C:/Users/threh/OneDrive/바탕 화면/000취준/엘아지/PT면접/미사일시스템SW_이장훈.pdf')
OUT=ROOT/'output/pdf/미사일시스템SW_이장훈_프리텐다드_수정.pdf';OUT.parent.mkdir(parents=True,exist_ok=True)
r=PdfReader(SRC);w=PdfWriter();navy='#1A396C';dark='#252525';gray='#717171';light='#EEEEEE';H=1134
count=0

def rect(x,y,ww,hh,color='white',stroke=None):
 c.setFillColor(color); c.setStrokeColor(stroke or color); c.setLineWidth(1.3); c.rect(x,H-y-hh,ww,hh,fill=1,stroke=bool(stroke))
def line(x,y,x2,y2,color='#A0A0A0',width=1):
 c.setStrokeColor(color);c.setLineWidth(width);c.line(x,H-y,x2,H-y2)
def txt(x,y,ww,hh,s,size=20,bold=False,color=dark,bg='white',align='left'):
 global count
 if bg:rect(x,y,ww,hh,bg)
 f='PB' if bold else 'PR';c.setFont(f,size);c.setFillColor(color)
 lines=s.split('\n'); leading=size*1.2
 for j,t in enumerate(lines):
  sw=pdfmetrics.stringWidth(t,f,size)
  if sw>ww-4:
   fs=size*(ww-4)/sw;c.setFont(f,fs)
  else:fs=size;c.setFont(f,size)
  xx=x+(ww-pdfmetrics.stringWidth(t,f,fs))/2 if align=='center' else x+2
  c.drawString(xx,H-y-fs*.88-j*leading,t);count+=1

def bracket(x,y,hh,right=False):
 dx=-5 if right else 5
 line(x+dx,y,x,y,dark,1.2);line(x,y,x,y+hh,dark,1.2);line(x,y+hh,x+dx,y+hh,dark,1.2)
def paren(x,y,hh,right=False):
 c.setStrokeColor(dark);c.setLineWidth(1.2)
 pth=c.beginPath();end=x-6 if right else x+6;pth.moveTo(end,H-y);pth.curveTo(x,H-y-hh*.15,x,H-y-hh*.85,end,H-y-hh);c.drawPath(pth)
def table(x,y,widths,heights,rows,header=True,leftdark=False):
 yy=y
 for ri,row in enumerate(rows):
  xx=x
  for ci,s in enumerate(row):
   bg=dark if (header and ri==0) or (leftdark and ci==0) else 'white'
   if ri==0 and ci==0 and leftdark:bg='#E3E9E7'
   rect(xx,yy,widths[ci],heights[ri],bg,'#8D8D8D')
   n=len(s.split('\n'));sz=21 if max(widths)>300 else 22
   txt(xx+3,yy+(heights[ri]-sz*1.2*n)/2,widths[ci]-6,sz*1.2*n,s,sz,ri==0,'white' if bg==dark else gray,None,'center')
   xx+=widths[ci]
  yy+=heights[ri]

for pi,p in enumerate(r.pages):
 b=BytesIO();c=canvas.Canvas(b,pagesize=(1440,810));c.scale(1/1.4,1/1.4)
 if pi==0:
  rect(1495,707,447,345)
  txt(1497,712,430,29,'주요 이력',24,True,navy)
  line(1498,749,1936,749,'#D7DDE5')
  entries=[(774,'2022','삼성전자 DS 장기현장실습'),(814,'2022','바이오메카트로닉스학과 졸업논문 은상'),(854,'2023','소프트웨어학과 졸업작품 은상'),(894,'2025','성균관대학교 학사 졸업\n바이오메카트로닉스학과 / 소프트웨어학과'),(963,'2025','개인사업자 등록'),(1003,'2026','판교 글로벌게임허브센터\n게임벤처 4.0 선정')]
  for y,yr,s in entries:
   txt(1497,y+(9 if '\n' in s else 0),65,27,yr,20,True,navy)
   txt(1567,y,371,49 if '\n' in s else 29,s,20)
 if pi==1:
  # Replace labels inside existing architecture boxes.
  for y in [203,391]:txt(773,y,150,31,'STM32' if y==203 else 'Zynq',27,True)
  for off in [0,188]:
   y=253+off
   txt(778,y+8,110,29,'UART IRQ',20,True,bg=light,align='center')
   txt(778,y+39,110,25,'바이트 수신',17,color=gray,bg=light,align='center')
   txt(920,y+12,60,49,'바이트\n큐',18,True,bg=light,align='center')
   txt(1018,y-23,145,29,'수신 태스크',21,True,align='center')
   txt(1025,y+15,130,25,'프레임 해석',18,True,'white',dark,'center')
   txt(1025,y+39,130,20,'조립 · CRC',15,color='#CCCCCC',bg=dark,align='center')
   txt(1200,y+13,81,26,'목표 큐',20,True,bg=light,align='center')
   txt(1200,y+40,81,22,'TargetState',14,color=gray,bg=light,align='center')
   txt(1322,y+9,101,29,'큐 확인',20,True,bg=light,align='center')
   txt(1322,y+39,101,25,'시각 취득',17,color=gray,bg=light,align='center')
  txt(1317,183,360,31,'제어 태스크  (FreeRTOS)',22,True)
  txt(1770,184,168,24,'제어 주기 설정: 20 ms',16,color=gray,align='center')
  txt(1819,216,111,28,'보드 차이',20,True,align='center')
  txt(1476,248,288,35,'공통 제어·매핑',24,True,'white',dark,'center')
  txt(1476,287,288,27,'동일 소스 · 보드별 독립 실행',18,color='#CCCCCC',bg=dark,align='center')
  for x,s,sub in [(1485,'제어기','조향각 계산'),(1645,'SG90 매핑','PWM 명령')]:
   txt(x,350,113,33,s,23,True,'white',dark,'center');txt(x,391,113,29,sub,18,color='#CCCCCC',bg=dark,align='center')
  txt(1490,465,107,29,'제어 요구',20,True,'white',dark,'center');txt(1646,465,111,29,'구동기 특성',20,True,'white',dark,'center')
  txt(1606,355,30,20,'deg',14,color='#BBBBBB',bg=dark,align='center')
  txt(1789,349,17,23,'μs',14,color=gray,align='center')
  for y,sub in [(253,'TIM4'),(441,'TTC 후속 확장')]:
   txt(1835,y+9,97,28,'PWM 출력',19,True,bg=light,align='center');txt(1835,y+40,97,22,sub,15,color=gray,bg=light,align='center')
  txt(760,557,537,25,'호스트 시험: CRC·포화·입력 유지·timeout·매핑 반전 등, CTest 7개 대상 통과',15,color=gray)
  rect(1524,578,427,35)
  for x,col,label in [(1531,dark,'공통 소스'),(1684,light,'플랫폼 구현'),(1841,'white','태스크 경계')]:
   rect(x,587,17,17,col,'#AAAAAA' if col=='white' else None);txt(x+24,584,113,24,label,18)
  # Small system flow: retain original photos and arrows.
  labels=[(1042,794,70,15,'보드별 개별 연결',10,False),(777,937,90,15,'MATLAB / Simulink',10,True),(782,952,82,14,'시뮬레이션 입력',9,False),(859,905,39,16,'UDP',10,True),(920,938,41,15,'Linux',11,True),(919,953,46,14,'C++ 패킷화',9,False),(1007,849,40,16,'UART',10,True),(1007,963,40,16,'UART',10,True),(1011,909,60,16,'115200 · 8N1',9,False),(1045,878,53,17,'STM32',12,True),(1034,894,81,14,'STM32F429I-DISC1',8,False),(1051,994,47,16,'Zynq',12,True),(1046,1009,57,14,'Zybo Z7-10',9,False),(1116,847,40,16,'PWM',10,True),(1109,863,54,14,'50 Hz · 20 ms',8,False),(1116,964,40,16,'PWM',10,True),(1113,980,42,14,'후속 확장',9,False),(1196,936,49,17,'서보모터',11,True),(1203,953,37,15,'SG90',10,False)]
  for x,y,ww,hh,s,sz,bo in labels:txt(x,y,ww,hh,s,sz,bo,align='center')
  # Result callout rebuilt with vector type and shapes.
  rect(1307,813,627,172)
  c.setFillColor(light);c.roundRect(1314,H-945,599,120,60,fill=1,stroke=0)
  c.setFillColor(dark);c.roundRect(1314,H-945,243,120,60,fill=1,stroke=0);rect(1314,825,122,120,dark)
  txt(1330,838,184,23,'공통 제어·매핑',16,True,'white',dark)
  txt(1332,898,50,28,'수정',21,True,'white',dark)
  txt(1398,851,65,85,'0',82,True,'white',dark,'center')
  txt(1465,900,40,27,'줄',21,True,'white',dark)
  txt(1584,846,260,25,'STM32 · Zynq',17,True,gray,light)
  txt(1584,879,306,40,'동일 PWM 명령값 일치',28,True,dark,light)
  txt(1311,959,611,22,'고정 입력 (515, −97) cm · PWM 명령: 주기 20,000 μs / 펄스 폭 1,388 μs',12,color=gray)
 if pi==2:
  table(27,734,[94,248],[36,59,59],[['계획','주요 역할'],['GPP','목적지까지 큰 경로 탐색'],['LPP','주변·차량 제약내\n속도·조향 계산']])
  for y,num,s in [(734,'1','차량 속도'),(797,'2','지도 변화의 지속성'),(860,'3','하드웨어 계산 여유')]:
   rect(395,y,307,55,'#E3E9E7');rect(395,y,65,55,dark)
   txt(401,y+15,52,28,num,22,True,'white',dark,'center');txt(483,y+15,215,29,s,22,bg='#E3E9E7')
  table(1427,186,[148,198,203],[51,68,77],[['GPP/LPP','기본 추종','우회통신'],['반복 재탐색','A','미실험'],['유효 경로 재사용','C','B']],True,True)
  txt(772,449,282,21,'4개 알고리즘 별, 두 임무 총 40회 유효 완주',12,color=gray)
  txt(1065,449,319,21,'표시 영역은 시험용 비용지도 교란',14,color=gray)
  txt(1427,449,535,21,'PC: Gazebo·인지·GPP  /  Raspberry Pi 4: LPP',13,color=gray)
  # Experiment sequence.
  rect(764,551,1234,117)
  steps=[(772,208,'01','LPP 알고리즘 선정 시험','4개 LPP알고리즘 비교\n개선 출발점 RPP 확보'),(1030,240,'02','A/B 첫 비교','두 정책 동시 변경\n효과의 원인 혼재'),(1288,240,'03','C 비교군 추가','A-C 재사용 효과\nC-B 우회 통신 대가'),(1545,240,'04','가중치 비교','3개의 가중치 후보 비교'),(1804,188,'05','D 반복 검증','상태별 이관 보강\n일시 변화·지속 막힘 시나리오')]
  for x,ww,n,title,body in steps:
   txt(x,559,28,20,n,14,color='#8398AD');line(x+33,568,x+ww,568,'#CCDCEC')
   txt(x,585,ww,31,title,23,True,'#2E597C');txt(x,626,ww+15,46,body,14)
  # Original sequence step 04 body says only candidate comparison; retain that exact content.
  # No additional masking needed.
  # Formula, labels and small footnotes.
  txt(801,768,511,19,'dᵢ: 차체 외곽과 관측 차단셀의 거리 / eᵢ: GPP 경로 편차',12,color=gray)
  rect(801,787,520,57)
  txt(807,806,120,30,'J(a) = 3 meanᵢ',18,bg=None)
  bracket(932,792,47)
  paren(944,792,47)
  txt(956,791,116,24,'max(0, 0.5 − dᵢ)',17,bg=None,align='center')
  line(956,816,1071,816,dark,1)
  txt(956,819,116,23,'0.5',17,bg=None,align='center')
  paren(1081,792,47,True)
  txt(1086,789,11,17,'2',13,bg=None)
  bracket(1105,792,47,True)
  txt(1115,806,98,29,'+ wₚ meanᵢ',18,bg=None)
  bracket(1218,792,47)
  paren(1230,792,47)
  txt(1240,792,29,24,'eᵢ',17,bg=None,align='center')
  line(1240,816,1269,816,dark,1)
  txt(1240,819,29,23,'0.2',17,bg=None,align='center')
  paren(1279,792,47,True)
  txt(1285,789,11,17,'2',13,bg=None)
  bracket(1306,792,47,True)
  txt(768,1015,559,24,'3개 후보 × 각 5회, 총 15회 비교 평균 시간·양 장치 CPU가 낮은 절충값',14,color=gray)
  txt(1348,780,644,24,'맵10 비용지도 교란 · 4조건 × 2사건 × 3회 = 24회 · Pi 4 연산',13,color=gray)
  # Results table (same column geometry).
  data=[['A → D 비교','일시 변화','지속 막힘'],['완주 (회/3)','3/3 → 3/3','2/3 → 3/3'],['장애물 여유 p05 (m)','0.535 → 1.356','0.367 → 0.306'],['PC GPP CPU (core·s)','69.22 → 49.92','70.40 → 50.71'],['Pi LPP 계산 p95 (ms)','4.89 → 11.25','5.00 → 45.40']]
  yy=812; widths=[233,220,212]
  for ri,row in enumerate(data):
   xx=1348; hh=37 if ri<4 else 36
   for ci,s in enumerate(row):
    bg=dark if ri==0 else 'white';rect(xx,yy,widths[ci],hh,bg,'#B6B6B6');txt(xx+11,yy+10,widths[ci]-20,25,s,16,ri==0,'white' if ri==0 else dark,bg);xx+=widths[ci]
   yy+=hh
  txt(1347,1016,650,23,'C도 6/6 완주 · 완주 외 성과 평균(A·B 지속 n=2) · 여유는 지도상 거리',12,color=gray)
 c.showPage();c.save();p.merge_page(PdfReader(b).pages[0]);w.add_page(p)
w.add_metadata({'/Title':'미사일시스템SW 이장훈 - 프리텐다드 텍스트 선명화','/Author':'이장훈'})
with OUT.open('wb') as f:w.write(f)
d=pdfium.PdfDocument(str(OUT))
for i in range(len(d)):d[i].render(scale=1.4).to_pil().save(str(ROOT/f'tmp/pdfs/final-{i+1}.png'))
print(OUT);print('text lines',count)

