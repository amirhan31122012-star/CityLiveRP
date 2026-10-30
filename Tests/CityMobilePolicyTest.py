from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityMobilePolicy import is_decorative_vehicle

class PolicyTest(unittest.TestCase):
    def test_known_decorative_cars(self):
        for name in ('Parking_Parking_Block_Cars', 'Parked Car.001', 'City_Vehicles', 'Street_Bus'):
            self.assertTrue(is_decorative_vehicle(name), name)
    def test_infrastructure_not_removed(self):
        for name in ('Wheel Stoppers', 'Parking Entrance', 'Parking', 'Road.003', 'Building_11', 'ScarletSign'):
            self.assertFalse(is_decorative_vehicle(name), name)

if __name__ == '__main__': unittest.main()
