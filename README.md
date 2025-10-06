# cute-video-player

Simple video player built with [Cute Framework](https://github.com/RandyGaul/cute_framework) and [pl_mpeg](https://github.com/phoboslab/pl_mpeg).

## Building

`./bootstrap` and then `./build`.

## Why?

Because Cute Framework is not playing [Bad Apple](https://archive.org/details/hd-60-fps-bad-apple-shadow-art-pv-720-x-960-60fps) yet and that has to be rectified.
On a more serious note, it is useful for playing short videos (tutorial, cutscene...) in a game.

[`cute_video.h`](src/cute_video.h) exposes a simple API to play a video from a [CF_File](https://randygaul.github.io/cute_framework/topics/virtual_file_system/).
It uses regular [`cute_draw`](https://randygaul.github.io/cute_framework/topics/drawing/) operations so the video can be scaled/repositioned or even have shader effects applied to.
