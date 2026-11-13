# kikit fab jlcpcb for the mixed panel: one schematic per board type, keyword refs prefixed KW_, plain keybit refs KB_
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from layout import KW_PREFIX, KB_PREFIX
import kikit.fab.jlcpcb as j
from kikit.eeschema_v6 import extractComponents

def both(_):
    hall = extractComponents("src/keybit-hall.kicad_sch")
    kw = extractComponents("src/keyword/keyword.kicad_sch")
    for c in kw:
        if not c.properties["Reference"].startswith("#"):
            c.properties["Reference"] = KW_PREFIX + c.properties["Reference"]
    kb = extractComponents("src/keybit/keybit.kicad_sch")
    for c in kb:
        if not c.properties["Reference"].startswith("#"):
            c.properties["Reference"] = KB_PREFIX + c.properties["Reference"]
    return hall + kw + kb

j.extractComponents = both
j.exportJlcpcb("2x3-test-panel.kicad_pcb", "production", True, "src/keybit-hall.kicad_sch",
               "", "LCSC Part,LCSC", "JLCPCB_CORRECTION", None, False, "{}", False, True)
