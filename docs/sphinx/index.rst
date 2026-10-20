.. OBS Studio documentation master file, created by
   sphinx-quickstart on Wed Oct 25 00:03:21 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. _contents:

Penguin Studio: inherited OBS API reference
==========================================

This reference documents the inherited OBS engine and plugin interfaces. API
names, upstream examples and attribution are retained for compatibility.
For Penguin Studio setup and releases, use the project guides below.

* **Building Penguin Studio?** `Use the developer guide <https://github.com/SigmaCodez/PenguinStudio/blob/main/CONTRIBUTING.md>`_
* **Installing Penguin Studio?** `Download a Penguin Studio release <https://github.com/SigmaCodez/PenguinStudio/releases>`_
* **Developing your first OBS plugin?** `Use the obs-plugintemplate <https://github.com/obsproject/obs-plugintemplate#obs-plugin-template>`_

.. toctree::
   :caption: Core Concepts
   :maxdepth: 1

   backend-design
   plugins
   frontends
   graphics
   scripting

.. toctree::
   :caption: API Reference
   :maxdepth: 2

   OBS Core <reference-core>
   Modules <reference-modules>
   Core API Object <reference-core-objects>
   Platform/Utility <reference-libobs-util>
   Callbacks (libobs/callback) <reference-libobs-callback>
   Graphics (libobs/graphics) <reference-libobs-graphics>
   Media I/O (libobs/media-io) <reference-libobs-media-io>
   reference-frontend-api

.. toctree::
   :caption: Additional Resources
   :maxdepth: 1
   :hidden:

   Penguin Studio Builds <https://github.com/SigmaCodez/PenguinStudio/blob/main/docs/PACKAGING.md>
   Plugin Template <https://github.com/obsproject/obs-plugintemplate#obs-plugin-template>