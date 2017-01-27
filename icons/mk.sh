# 26 january 2017
set -e -x
convert \
	StatusAnnotations_Complete_and_ok_16xLG_color.png \
	yes.ico
convert \
	StatusAnnotations_Critical_16xLG_color.png \
	no.ico
convert \
	StatusAnnotations_Help_and_inconclusive_16xLG_color.png \
	unknown.ico
