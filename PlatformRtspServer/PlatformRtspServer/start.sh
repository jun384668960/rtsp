#!/bin/bash

./rtspServer --dispatch=cnp2p.ulifecam.com:6001 --log=log/ --cert=cert.pem --key=key.pem --livesecs=60
