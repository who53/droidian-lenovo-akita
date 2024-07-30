#!/bin/bash

sed -i '
s/.*realtime-priority.*/realtime-priority = 5/g
s/.*realtime-scheduling.*/realtime-scheduling = yes/g
s/.*nice-level.*/;nice-level = -11/g
s/.*high-priority.*/;high-priority = yes/g
s/.*avoid-resampling.*/avoid-resampling = true/g
' /etc/pulse/daemon.conf
