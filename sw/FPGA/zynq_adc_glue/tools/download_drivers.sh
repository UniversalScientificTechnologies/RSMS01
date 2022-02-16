#!/usr/bin/env bash

ssh space.astro.cz "ssh root@parallella.tunnel tar c drivers" | tar x

