from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityMobilePolicy import is_decorative_vehicle, draw_distance

class PolicyTest(unittest.TestCase):
    def test_known_decorative_cars(self):
        for name in ('Parking_Parking_Block_Cars', 'Parked Car.001', 'City_Vehicles', 'Street_Bus'):
            self.assertTrue(is_decorative_vehicle(name), name)
    def test_infrastructure_not_removed(self):
        for name in ('Wheel Stoppers', 'Parking Entrance', 'Parking', 'Road.003', 'Building_11', 'ScarletSign'):
            self.assertFalse(is_decorative_vehicle(name), name)
    def test_ground_not_distance_culled(self):
        self.assertEqual(draw_distance('Road.003'), 0)
        self.assertEqual(draw_distance('Sidewalk'), 0)
        self.assertEqual(draw_distance('Parking_Trees_High_Poly'), 8000)

if __name__ == '__main__': unittest.main()
