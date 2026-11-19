{
 "patcher": {
  "fileversion": 1,
  "appversion": {
   "major": 9,
   "minor": 0,
   "revision": 0,
   "architecture": "x64",
   "modernui": 1
  },
  "classnamespace": "box",
  "rect": [
   80,
   80,
   1140,
   720
  ],
  "openinpresentation": 0,
  "default_fontsize": 12.0,
  "default_fontface": 0,
  "default_fontname": "Arial",
  "gridonopen": 1,
  "gridsize": [
   15.0,
   15.0
  ],
  "boxes": [
   {
    "box": {
     "id": "c-title",
     "maxclass": "comment",
     "text": "philotaxis moire sequencer \u2014 drag sliders, click toggles, double-click to reset. Messages: <param> <value>, start, stop, reset, defaults, panel 0/1",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      20,
      10,
      760,
      20
     ]
    }
   },
   {
    "box": {
     "id": "m-0",
     "maxclass": "message",
     "text": "start",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      20,
      38,
      49,
      22
     ]
    }
   },
   {
    "box": {
     "id": "m-1",
     "maxclass": "message",
     "text": "stop",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      79,
      38,
      42,
      22
     ]
    }
   },
   {
    "box": {
     "id": "m-2",
     "maxclass": "message",
     "text": "reset",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      131,
      38,
      49,
      22
     ]
    }
   },
   {
    "box": {
     "id": "m-3",
     "maxclass": "message",
     "text": "defaults",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      190,
      38,
      70,
      22
     ]
    }
   },
   {
    "box": {
     "id": "m-4",
     "maxclass": "message",
     "text": "panel 1",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      270,
      38,
      63,
      22
     ]
    }
   },
   {
    "box": {
     "id": "m-5",
     "maxclass": "message",
     "text": "panel 0",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      343,
      38,
      63,
      22
     ]
    }
   },
   {
    "box": {
     "id": "seq",
     "maxclass": "v8ui",
     "filename": "philotaxis_moire_sequencer.js",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      ""
     ],
     "parameter_enable": 0,
     "patching_rect": [
      20,
      70,
      760,
      520
     ],
     "presentation": 1,
     "presentation_rect": [
      0,
      0,
      760,
      520
     ]
    }
   },
   {
    "box": {
     "id": "mk",
     "maxclass": "newobj",
     "text": "makenote 100 150",
     "numinlets": 3,
     "numoutlets": 2,
     "outlettype": [
      "float",
      "float"
     ],
     "patching_rect": [
      20,
      610,
      110,
      22
     ]
    }
   },
   {
    "box": {
     "id": "no",
     "maxclass": "newobj",
     "text": "noteout",
     "numinlets": 3,
     "numoutlets": 0,
     "outlettype": [],
     "patching_rect": [
      20,
      650,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "c-no",
     "maxclass": "comment",
     "text": "double-click noteout to pick the MIDI port",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      90,
      650,
      260,
      20
     ]
    }
   },
   {
    "box": {
     "id": "c-sync",
     "maxclass": "comment",
     "text": "tempo sync: rpm = bpm / beats per revolution",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      800,
      70,
      280,
      20
     ]
    }
   },
   {
    "box": {
     "id": "c-bpm",
     "maxclass": "comment",
     "text": "bpm",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      800,
      92,
      40,
      20
     ]
    }
   },
   {
    "box": {
     "id": "c-beats",
     "maxclass": "comment",
     "text": "beats / rev",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      870,
      92,
      80,
      20
     ]
    }
   },
   {
    "box": {
     "id": "bpm",
     "maxclass": "flonum",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      "bang"
     ],
     "patching_rect": [
      800,
      112,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "beats",
     "maxclass": "flonum",
     "numinlets": 1,
     "numoutlets": 2,
     "outlettype": [
      "",
      "bang"
     ],
     "patching_rect": [
      870,
      112,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "pak",
     "maxclass": "newobj",
     "text": "pak 120. 16.",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      800,
      146,
      80,
      22
     ]
    }
   },
   {
    "box": {
     "id": "expr",
     "maxclass": "newobj",
     "text": "expr $f1 / $f2",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      800,
      178,
      90,
      22
     ]
    }
   },
   {
    "box": {
     "id": "pre",
     "maxclass": "newobj",
     "text": "prepend rpm",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      800,
      210,
      80,
      22
     ]
    }
   },
   {
    "box": {
     "id": "lb",
     "maxclass": "newobj",
     "text": "loadbang",
     "numinlets": 1,
     "numoutlets": 1,
     "outlettype": [
      "bang"
     ],
     "patching_rect": [
      800,
      260,
      60,
      22
     ]
    }
   },
   {
    "box": {
     "id": "c-lb",
     "maxclass": "comment",
     "text": "bang: send the MIDI channel to noteout",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      865,
      260,
      240,
      20
     ]
    }
   }
  ],
  "lines": [
   {
    "patchline": {
     "source": [
      "m-0",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m-1",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m-2",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m-3",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m-4",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "m-5",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "seq",
      0
     ],
     "destination": [
      "mk",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mk",
      0
     ],
     "destination": [
      "no",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "mk",
      1
     ],
     "destination": [
      "no",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "seq",
      1
     ],
     "destination": [
      "no",
      2
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "bpm",
      0
     ],
     "destination": [
      "pak",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "beats",
      0
     ],
     "destination": [
      "pak",
      1
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pak",
      0
     ],
     "destination": [
      "expr",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "expr",
      0
     ],
     "destination": [
      "pre",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "pre",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   },
   {
    "patchline": {
     "source": [
      "lb",
      0
     ],
     "destination": [
      "seq",
      0
     ]
    }
   }
  ]
 }
}