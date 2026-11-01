"""Tests the offline review prototype, not native Slate or an APK. Requires Playwright + Chromium."""
import asyncio,json,base64
from pathlib import Path
from playwright.async_api import async_playwright
root=Path(__file__).resolve().parents[1]
out=root/'Assets/UI'
async def main():
 async with async_playwright() as p:
  browser=await p.chromium.launch(executable_path='/usr/bin/chromium',headless=True,args=['--no-sandbox','--disable-dev-shm-usage'])
  page=await browser.new_page(viewport={'width':1280,'height':750},device_scale_factor=1)
  errors=[];page.on('pageerror',lambda e:errors.append(str(e)))
  html=(out/'InterfacePreview.html').read_text()
  for relative in ('../Branding/CityLiveLoginCity.png','../Maps/PoliceDistrictRealistic/StreetRealistic.png'):
   html=html.replace(relative,'data:image/png;base64,'+base64.b64encode((out/relative).read_bytes()).decode())
  await page.set_content(html);await page.wait_for_load_state('networkidle')
  assert await page.locator('input[type=email]').count()==0
  await page.locator('#first').fill('Alex');await page.locator('#last').fill('Ivanov');await page.locator('#next').click()
  assert await page.locator('h1').inner_text()=='Регистрация'
  await page.locator('#password').fill('secret1');await page.locator('#confirm').fill('secret2');await page.locator('#auth').click()
  assert 'не совпадают' in await page.locator('#error').inner_text()
  await page.locator('#confirm').fill('secret1');await page.locator('#auth').click()
  assert await page.locator('h1').inner_text()=='Место появления'
  await page.get_by_role('button',name='Последний выход',exact=False).click();assert await page.evaluate('chosenSpawn')
  await page.get_by_role('button',name='ДАЛЕЕ →').click();assert await page.evaluate('scene')=='hud'
  await page.get_by_role('button',name='GPS',exact=True).click()
  await page.locator('[data-group=Government]').click();await page.locator('[data-place=Hospital]').click();await page.locator('#followgps').click()
  assert await page.evaluate('tracked.id')=='Hospital'
  await page.locator('.mini').click();assert await page.evaluate('scene')=='map'
  await page.locator('#track').click();assert await page.evaluate('tracked') is None
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
  assert not errors,errors
  print('PASS: offline design prototype — name/password/spawn flow, no email, GPS selection, route cancellation, RU/EN keyboard and /gps; 9 review images')
  await browser.close()
asyncio.run(main())
