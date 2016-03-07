# AutoLoader
Automated 3DS CIA installer/updater that operates from a web backend

To use with your own app, write the URL to the sd card as /autoloader.url and APT_DoAppJump() over.

This requires my fork of [citrus](https://github.com/ksanislo/citrus/) which provides an overloaded version of ctr::app::install() that can accept an httpcContext.
