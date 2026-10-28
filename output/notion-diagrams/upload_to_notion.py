"""Send one multipart POST using the short-lived Notion upload ticket on stdin."""
import json
import sys
import uuid
import urllib.request
import urllib.error
from pathlib import Path

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None

ticket = json.loads(sys.stdin.read().lstrip('\ufeff'))
root = Path(__file__).resolve().parent
path = root / ticket['filename']
assert path.parent == root and path.suffix == '.png'
assert ticket['upload_method'] == 'POST'
boundary = 'NotionDiagram' + uuid.uuid4().hex
body = (f'--{boundary}\r\nContent-Disposition: form-data; name="file"; '
        f'filename="{path.name}"\r\nContent-Type: image/png\r\n\r\n').encode()
body += path.read_bytes() + f'\r\n--{boundary}--\r\n'.encode()
headers = dict(ticket['upload_headers'])
headers['Content-Type'] = 'multipart/form-data; boundary=' + boundary
request = urllib.request.Request(ticket['upload_url'], data=body, headers=headers, method='POST')
try:
    with urllib.request.build_opener(NoRedirect()).open(request, timeout=50) as response:
        result = json.loads(response.read())
except urllib.error.HTTPError as error:
    print(json.dumps({'ok': False, 'filename': path.name, 'http_status': error.code}))
    sys.exit(1)
except urllib.error.URLError as error:
    print(json.dumps({'ok': False, 'filename': path.name, 'error_type': type(error).__name__, 'reason': str(error.reason)}))
    sys.exit(1)
except Exception as error:
    print(json.dumps({'ok': False, 'filename': path.name, 'error_type': type(error).__name__}))
    sys.exit(1)
(root / (path.stem + '.upload.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result, ensure_ascii=True))
