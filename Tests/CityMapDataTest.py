"""Guard the UI map against drift from the packaged geometry and live entrance coordinates."""
import hashlib,json,re,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class MapDataTest(unittest.TestCase):
 def test_geometry_source_matches_packaged_city(self):
  data=json.loads((ROOT/'Assets/UI/DistrictMap.json').read_text())
  package=json.loads((ROOT/'Assets/Maps/PoliceDistrictGame/manifest.json').read_text())
  source=ROOT/'Assets/Maps/PoliceDistrictRealistic/CityLive_PoliceDistrict_Realistic.blend'
  self.assertEqual(data['source_blend_sha256'],package['source_blend_sha256'])
  self.assertEqual(data['source_blend_sha256'],hashlib.sha256(source.read_bytes()).hexdigest())
  self.assertEqual(len(data['roads']),8);self.assertEqual(sum(bool(r.get('access')) for r in data['roads']),1)
  self.assertGreater(len(data['walkways']),20)
 def test_named_waypoints_are_real_runtime_entrances(self):
  places={p['id']:p for p in json.loads((ROOT/'Assets/UI/DistrictMap.json').read_text())['places']}
  world=(ROOT/'Source/CityLiveRP/CityDistrictWorld.cpp').read_text()
  for name,x,y in re.findall(r'\{TEXT\("([^"]+)"\),FVector\(([-\d.]+),([-\d.]+),100\),\d+,\d+\}',world):
   self.assertEqual(places[name]['point'],[float(x)/100,float(y)/100],name)
  for variable,name in [('PoliceDoor','Police'),('HospitalDoor','Hospital'),('CourierDesk','Courier')]:
   match=re.search(variable+r'\(([-\d.]+),([-\d.]+),100\)',world)
   self.assertEqual(places[name]['point'],[float(match[1])/100,float(match[2])/100],name)
 def test_review_routes_are_continuous_and_finish_at_their_waypoint(self):
  data=json.loads((ROOT/'Assets/UI/DistrictMap.json').read_text());places={p['id']:p for p in data['places']}
  routes=[json.loads(s) for s in (ROOT/'Assets/UI/RoutePreview.jsonl').read_text().splitlines()]
  self.assertEqual(len(routes),len(places))
  for r in routes:
   self.assertEqual(len(r['road']),len(r['points'])-1)
   self.assertAlmostEqual(r['points'][-1][0],places[r['id']]['point'][0],places=3)
   self.assertAlmostEqual(r['points'][-1][1],places[r['id']]['point'][1],places=3)
if __name__=='__main__':unittest.main()
