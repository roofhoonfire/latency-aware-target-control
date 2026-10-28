from PIL import Image
im=Image.open('tmp/pdfs/original-3.png'); im.crop((1345,809,2016,1036)).resize((2013,681)).save('tmp/pdfs/results.png')
