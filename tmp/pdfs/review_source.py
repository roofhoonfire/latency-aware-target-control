from pathlib import Path
from pypdf import PdfReader
import pypdfium2 as pdfium
src=Path(r'C:/Users/threh/OneDrive/바탕 화면/000취준/엘아지/PT면접/미사일시스템SW_이장훈.pdf')
r=PdfReader(src); d=pdfium.PdfDocument(str(src))
for i,p in enumerate(r.pages):
 print('PAGE',i+1, p.mediabox, 'IMAGES',len(p.images)); print(p.extract_text())
 d[i].render(scale=1.4).to_pil().save(f'tmp/pdfs/original-{i+1}.png')
