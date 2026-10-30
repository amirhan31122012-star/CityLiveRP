"""Compatibility entry point: the city update now restores V1 graphics."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parent))
from RestoreCityV1 import main

main()
