#!/usr/bin/env bash

tar c drivers | ssh space.astro.cz "ssh root@parallella.tunnel tar x"

