
ipcs -m | awk '$2 ~ /[0-9]+/ {print $2}' | while read s; do sudo ipcrm -m $s; done