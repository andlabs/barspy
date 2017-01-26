# 26 january 2017
set -e -x
convert \
	StatusAnnotations_Complete_and_ok_16xLG_color.png \
	StatusAnnotations_Complete_and_ok_32xLG_color.png \
	yes.ico
convert \
	StatusAnnotations_Critical_16xLG_color.png \
	StatusAnnotations_Critical_32xLG_color.png \
	no.ico
