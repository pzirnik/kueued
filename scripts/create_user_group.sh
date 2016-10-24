#!/bin/bash
groupadd kueued
useradd -c "kueued user" -s /bin/false -G kueued kueued
