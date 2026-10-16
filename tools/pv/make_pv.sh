#!/usr/bin/env bash
# 絵コンテのカット表(pv_cuts.tsv)から PV を書き出す。Git Bash で実行:
#   bash tools/pv/make_pv.sh [カット表] [出力ファイル]
# 出力の既定は ~/Videos/PV/Eclipser_PV.mp4（720p60 / 約5Mbps）。
set -euo pipefail

FF="${FF:-/c/Program Files (x86)/Digiarty/VideoProc Converter AI/ffmpeg.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
CUTS="$(realpath "${1:-$HERE/pv_cuts.tsv}")"
OUT="${2:-$HOME/Videos/PV/Eclipser_PV.mp4}"
BGM_VOL="${BGM_VOL:-0.55}"
WORK="$(dirname "$OUT")/work"

mkdir -p "$WORK"
rm -f "$WORK"/seg*.mkv "$WORK"/run*.wav "$WORK"/t*.txt
cp "$REPO/Resource/Font/Noto_Sans_JP/static/NotoSansJP-Bold.ttf" "$WORK/font.ttf"
cp "$HERE/credits.txt" "$WORK/credits.txt"

ff() { "$FF" -hide_banner -loglevel error -y "$@"; }
enc=(-c:v libx264 -preset veryfast -crf 16 -pix_fmt yuv420p -c:a pcm_s16le -ar 48000 -ac 2)

# テロップ（前後0.4秒でフェード）
telop() { # id dur pos text
	local id=$1 dur=$2 pos=$3 text=$4 x y fs
	[ "$text" = "-" ] && return 0
	printf '%s' "$text" > "$WORK/t$id.txt"
	if [ "$pos" = "bc" ]; then fs=24; x='(w-tw)/2'; y='h-th-56'
	else fs=26; x='w-tw-56'; y='h-th-44'; fi
	printf ",drawtext=fontfile=font.ttf:textfile=t%s.txt:fontsize=%s:fontcolor=white:shadowcolor=black@0.75:shadowx=2:shadowy=2:x=%s:y=%s:alpha='if(lt(t,0.4),t/0.4,if(gt(t,%s-0.4),(%s-t)/0.4,1))'" \
		"$id" "$fs" "$x" "$y" "$dur" "$dur"
}

cd "$WORK"
: > list.txt
runs=() ; run_bgm="" ; run_dur=0
total=0
while IFS=$'\t' read -r id dur src start vol bgm pos text label; do
	[[ -z "$id" || "$id" == \#* ]] && continue
	seg=$(printf 'seg%02d.mkv' "$id")
	tf=$(telop "$id" "$dur" "$pos" "$text")
	case "$src" in
	placeholder)
		printf '撮影待ち｜PV %s\n%s' "$id" "$label" > "p$id.txt"
		ff -f lavfi -i "color=c=0x101014:s=1280x720:r=60:d=$dur" -f lavfi -i anullsrc=r=48000:cl=stereo \
			-vf "drawtext=fontfile=font.ttf:textfile=p$id.txt:fontsize=30:fontcolor=0x8a8a96:line_spacing=12:x=(w-tw)/2:y=(h-th)/2$tf" \
			-t "$dur" "${enc[@]}" "$seg" ;;
	black)
		ff -f lavfi -i "color=c=black:s=1280x720:r=60:d=$dur" -f lavfi -i anullsrc=r=48000:cl=stereo \
			-vf "null$tf" -t "$dur" "${enc[@]}" "$seg" ;;
	card)
		# credits.txt は1行ずつ中央揃えで描く
		printf 'Eclipser' > title.txt
		vf="drawtext=fontfile=font.ttf:textfile=title.txt:fontsize=72:fontcolor=white:x=(w-tw)/2:y=h*0.26"
		n=0
		while IFS= read -r line || [ -n "$line" ]; do
			[ -z "$line" ] && continue
			printf '%s' "$line" > "c$n.txt"
			vf+=",drawtext=fontfile=font.ttf:textfile=c$n.txt:fontsize=24:fontcolor=0xd0d0d8:x=(w-tw)/2:y=h*0.5+$((n*38))"
			n=$((n+1))
		done < credits.txt
		ff -f lavfi -i "color=c=black:s=1280x720:r=60:d=$dur" -f lavfi -i anullsrc=r=48000:cl=stereo \
			-vf "$vf" -t "$dur" "${enc[@]}" "$seg" ;;
	*)
		[[ "$src" = /* || "$src" =~ ^[A-Za-z]: ]] || src="$HERE/$src"
		ff -ss "$start" -t "$dur" -i "$src" -f lavfi -i anullsrc=r=48000:cl=stereo \
			-filter_complex "[0:v]scale=1280:720:flags=lanczos,fps=60,setsar=1$tf[v];[0:a]volume=$vol[a0];[a0][1:a]amix=inputs=2:duration=first:normalize=0[a]" \
			-map "[v]" -map "[a]" -t "$dur" "${enc[@]}" "$seg" ;;
	esac
	echo "file '$seg'" >> list.txt
	total=$(awk "BEGIN{print $total+$dur}")
	# BGM: 同じ文字列が続く間は1本で流す
	if [ "$bgm" = "$run_bgm" ]; then run_dur=$(awk "BEGIN{print $run_dur+$dur}")
	else
		[ -n "$run_bgm" ] && runs+=("$run_bgm|$run_dur")
		run_bgm=$bgm; run_dur=$dur
	fi
	echo "  cut $id ($dur s) ok"
done < "$CUTS"
runs+=("$run_bgm|$run_dur")

: > bgm.txt
k=0
for r in "${runs[@]}"; do
	name=${r%%|*}; d=${r##*|}; w=$(printf 'run%02d.wav' $k)
	if [ "$name" = "-" ]; then
		ff -f lavfi -i anullsrc=r=48000:cl=stereo -t "$d" -c:a pcm_s16le "$w"
	else
		file=${name%@*}; off=0; [[ "$name" == *@* ]] && off=${name##*@}
		ff -ss "$off" -t "$d" -i "$REPO/Resource/sound/$file" \
			-af "apad,atrim=0:$d,afade=t=in:d=0.5,afade=t=out:st=$(awk "BEGIN{print $d-1.5}"):d=1.5" \
			-ar 48000 -ac 2 -c:a pcm_s16le "$w"
	fi
	echo "file '$w'" >> bgm.txt; k=$((k+1))
done

ff -f concat -safe 0 -i list.txt -c copy body.mkv
ff -f concat -safe 0 -i bgm.txt -c copy bgm.wav
fo=$(awk "BEGIN{print $total-1.2}")
ff -i body.mkv -i bgm.wav \
	-filter_complex "[0:v]fade=t=in:d=0.8,fade=t=out:st=$fo:d=1.2[v];[1:a]volume=$BGM_VOL[b];[0:a][b]amix=inputs=2:duration=first:normalize=0,afade=t=out:st=$fo:d=1.2[a]" \
	-map "[v]" -map "[a]" -c:v libx264 -preset slow -b:v 5M -maxrate 6M -bufsize 10M -pix_fmt yuv420p \
	-c:a aac -b:a 192k -movflags +faststart "$OUT"
echo "done: $OUT (${total}s)"
