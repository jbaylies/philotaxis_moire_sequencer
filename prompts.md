
Let's create a philotaxis sequencer based on /Users/jbaylies/Documents/sousastep.quest/public/shaders

It shall be made for MaxMSP.

The UI shall use Max's v8ui object which provides an environment to make user interface elements using Javascript (ECMAScript 6+). This provides all of the programming tools available in the v8 object, but also exposes the mgraphics and sketch drawing routines for visual output. 

Seems like someone else had Claude make 100 sequencers for v8ui in Max a year ago, which I've cloned here as a basic reference /Users/jbaylies/Desktop/philotaxis_moire_sequencer/max-v8ui-sequencers do not add to this repository. I'll make a new one later, if this is worth saving.

A vertical line shall be drawn from the center to the edge. The vertical line can rotate around the center at a speed set by a parameter. Whenever the line touches a point, a chromatic MIDI note plays, in a note range set by two min max parameters. The volume of the MIDI note is determined simply by the brightness of the color of the point. The length of the MIDI notes is the same for all, set by a parameter.

The parameters should include all parameters displayed in the UI for the shader.


