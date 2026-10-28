from pathlib import Path
p=Path('tmp/pdfs/retype_pdf.py');s=p.read_text(encoding='utf-8')
s=s.replace('def table(x,y,widths', '''def bracket(x,y,hh,right=False):
 dx=-5 if right else 5
 line(x+dx,y,x,y,dark,1.2);line(x,y,x,y+hh,dark,1.2);line(x,y+hh,x+dx,y+hh,dark,1.2)
def paren(x,y,hh,right=False):
 c.setStrokeColor(dark);c.setLineWidth(1.2)
 pth=c.beginPath();end=x-6 if right else x+6;pth.moveTo(end,H-y);pth.curveTo(x,H-y-hh*.15,x,H-y-hh*.85,end,H-y-hh);c.drawPath(pth)
def table(x,y,widths''')
for old,new in [
("txt(928,790,14,54,'[',47,bg=None)","bracket(932,792,47)"),
("txt(941,790,13,54,'(',47,bg=None)","paren(944,792,47)"),
("txt(1073,790,13,54,')',47,bg=None)","paren(1081,792,47,True)"),
("txt(1098,790,14,54,']',47,bg=None)","bracket(1105,792,47,True)"),
("txt(1214,790,10,54,'[',47,bg=None)","bracket(1218,792,47)"),
("txt(1227,790,11,54,'(',47,bg=None)","paren(1230,792,47)"),
("txt(1272,790,11,54,')',47,bg=None)","paren(1279,792,47,True)"),
("txt(1298,790,14,54,']',47,bg=None)","bracket(1306,792,47,True)")]:s=s.replace(old,new)
p.write_text(s,encoding='utf-8')
