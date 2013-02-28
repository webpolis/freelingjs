#!/bin/bash
node-gyp configure && node-gyp build && cp ~/workspace/nico/nodejs/node_modules/freeling/build/Release/freeling.node ~/workspace/zempty-webpolis/src/modules/freeling.node
