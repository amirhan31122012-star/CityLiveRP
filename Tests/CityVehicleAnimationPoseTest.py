import json,math,subprocess,unittest
from pathlib import Path
from Scripts.CityVehicleAnimationPose import sample,phases,DURATION,rotate,qinv,sub,length,dot

ROOT=Path(__file__).resolve().parents[1]
class VehicleAnimationPoseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.reference=json.loads((ROOT/'Tests/Fixtures/CityBodyReference.json').read_text())

    def test_all_frames_preserve_bind_lengths_and_unit_scale(self):
        ref=self.reference
        for clip in ('Enter','Exit','EnterRight','ExitRight','DriverIdle'):
            previous=None
            for index in range(205):
                pose=sample(ref,index/30,clip,forward=(0,-1,0))
                for bone,(pos,rot,scale) in pose.items():
                    self.assertEqual(scale,(1,1,1))
                    self.assertAlmostEqual(length(rot),1,places=6)
                    parent=ref[bone]['parent']
                    expected=rotate(qinv(ref[parent]['rot']),sub(ref[bone]['pos'],ref[parent]['pos'])) if parent else ref[bone]['pos']
                    self.assertLess(length(sub(pos,expected)),1e-5,bone)
                    if previous:
                        # Sampling is continuous; no instant pose or root displacement.
                        angle=2*math.acos(min(1,abs(dot(rot,previous[bone][1]))))
                        self.assertLess(angle,math.radians(12),f'{clip} {bone} {index}')
                previous=pose

    def test_clip_endpoints_match_without_snap(self):
        ref=self.reference
        standing=sample(ref,0,'Enter',forward=(0,-1,0))
        idle=sample(ref,0,'DriverIdle',forward=(0,-1,0))
        for right in ('','Right'):
            seated=sample(ref,DURATION,'Enter'+right,forward=(0,-1,0))
            exit_start=sample(ref,0,'Exit'+right,forward=(0,-1,0))
            exit_end=sample(ref,DURATION,'Exit'+right,forward=(0,-1,0))
            for bone in ref:
                for a,b in ((idle[bone],seated[bone]),(idle[bone],exit_start[bone]),(standing[bone],exit_end[bone])):
                    self.assertLess(length(sub(a[0],b[0])),1e-6)
                    self.assertGreater(abs(dot(a[1],b[1])),.999999)

    def test_python_clip_clock_matches_native_entry_clock(self):
        output=subprocess.check_output(['/tmp/city-full-entry','--dump'],text=True)
        keys=('door','hand','sit','first','second','shift','turn','lean','wheel')
        for line in output.splitlines():
            direction,t,*values=map(float,line.split())
            expected=phases(t,bool(direction))
            for key,value in zip(keys,values):self.assertAlmostEqual(value,expected[key],delta=3e-6)

if __name__=='__main__':unittest.main()
