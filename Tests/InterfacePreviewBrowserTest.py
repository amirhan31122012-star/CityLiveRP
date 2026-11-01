"""Exercise the independent design prototype. This does not compile or run Unreal."""
import asyncio,json,base64,re,shutil
from pathlib import Path
from playwright.async_api import async_playwright
root=Path(__file__).resolve().parents[1];out=root/'Assets/UI'
async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path=shutil.which('chromium'),headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  page=await browser.new_page(viewport={'width':1280,'height':750},device_scale_factor=1)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  html=(out/'InterfacePreview.html').read_text()
  paths=set(re.findall(r"(?:\.\./Branding/|\.\./Maps/|Spawn/)[\w/.-]+\.png",html))
  assert len(paths)==8,paths
  # Render owned files in memory; the hosted browser's policy blocks file://.
  for relative in paths:html=html.replace(relative,'data:image/png;base64,'+base64.b64encode((out/relative).read_bytes()).decode())
  await page.set_content(html);await page.wait_for_load_state('networkidle')
  assert await page.locator('input[type=email]').count()==0
  assert await page.locator('.panel').count()==0
  assert await page.locator('#next').inner_text()=='ИГРАТЬ'
  a=await page.locator('#first').bounding_box();b=await page.locator('#last').bounding_box()
  assert a['height']==b['height']==76 and b['y']>a['y']+a['height']
  await page.locator('#first').fill('Alex');await page.locator('#last').fill('Ivanov');await page.locator('#next').click()
  assert await page.locator('h1').inner_text()=='РЕГИСТРАЦИЯ'
  assert 'accountart' in await page.locator('#screen').get_attribute('class')
  await page.locator('#password').fill('secret1');await page.locator('#confirm').fill('secret2');await page.locator('#auth').click()
  assert 'не совпадают' in await page.locator('#error').inner_text()
  await page.locator('#confirm').fill('secret1');await page.locator('#auth').click()
  assert await page.locator('h1').inner_text()=='Выберите место появления'
  assert await page.locator('.spawncard').count()==5
  assert await page.locator('[data-spawn=Home]').is_disabled()
  await page.locator('[data-spawn=Last]').click();assert await page.evaluate('chosenSpawn')=='Last'
  await page.locator('.spawnenter').click();assert await page.evaluate('scene')=='hud'
  assert await page.locator('.balance').inner_text()=='20 000'
  assert await page.locator('.balance').evaluate("e=>getComputedStyle(e).backgroundColor")=='rgba(0, 0, 0, 0)'
  assert not await page.get_by_role('button',name='GPS',exact=True).count()
  assert 'CITYLIVE' not in await page.locator('#screen').inner_text()
  mini=await page.locator('.mini').bounding_box();chat=await page.locator('.chatbox').bounding_box()
  assert mini['x']>chat['x']+chat['width'] and chat['height']==218 and mini['width']==218
  await page.locator('.mini').click()
  assert not await page.locator('.mapinfo').count()
  assert 'ВЫБЕРИТЕ ТОЧКУ' not in await page.locator('#screen').inner_text()
  await page.locator('.map .poi[data-id=Hospital]').click()
  assert await page.locator('#track').inner_text()=='ПОСТРОИТЬ МАРШРУТ'
  await page.locator('#track').click();assert await page.evaluate('tracked.id')=='Hospital'
  await page.locator('#track').click();assert await page.evaluate('tracked') is None
  await page.get_by_role('button',name='GPS',exact=True).click()
  assert 'Открыть карту района' not in await page.locator('#screen').inner_text()
  await page.locator('[data-group=Government]').click();await page.locator('[data-place=Hospital]').click();await page.locator('#followgps').click()
  assert await page.evaluate('tracked.id')=='Hospital'
  await page.evaluate("show('chat')")
  for key in ('/','g','p','s'):await page.locator('.key').get_by_text(key,exact=True).click()
  await page.locator('.key').get_by_text('→',exact=True).click();assert await page.evaluate('scene')=='gps'
  await page.evaluate("show('chat')");await page.locator('.key').get_by_text('EN',exact=True).click()
  for key in 'привет':await page.locator('.key').get_by_text(key,exact=True).click()
  await page.locator('.key').get_by_text('⌫',exact=True).click();assert await page.evaluate('draft')=='приве'
  for scene,name in [('login','Login'),('password','Registration'),('return','Authorization'),('spawn','Spawn'),('hud','HUD'),('gps','GPS'),('map','Map'),('chat','Keyboard')]:
   await page.evaluate("first='';last='';nickname='';draft='';gpsGroup='';tracked=DATA.places.find(p=>p.id==='Hospital');selectedPlace=null;show("+json.dumps(scene)+")")
   await page.screenshot(path=str(out/('Preview_'+name+'.png')))
  await page.set_viewport_size({'width':1600,'height':750});await page.evaluate("show('hud')");await page.screenshot(path=str(out/'Preview_WideHUD.png'))
  await page.evaluate("show('spawn')");cards=await page.locator('.spawncards').bounding_box()
  assert abs(cards['x']-52)<1 and abs(cards['width']-1496)<1,cards
  await page.evaluate("show('chat')");rect=await page.locator('.keyboard').bounding_box()
  assert abs(rect['x'])<1 and abs(rect['width']-1600)<1 and abs(rect['y']+rect['height']-720)<1,rect
  await page.screenshot(path=str(out/'Preview_WideKeyboard.png'))
  assert not errors,errors
  print('PASS: separate fields, full-screen password scene, five spawn cards, plain balance, right mini-map, map/GPS routing, full-width RU/EN keyboard; 10 review images')
  await browser.close()
asyncio.run(main())
