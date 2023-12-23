#!/opt/local/bin/bash
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

systype=$(uname -s)
case $systype in
  Darwin)
    ;;
  *)
    echo "Platform not supported"
    exit 1
    ;;
esac

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <directory>"
  exit 1
fi

outdir=$1
if [[ ! -d $outdir ]]; then
  echo "$outdir is not a directory"
  exit 1
fi

declare -A vlist
declare -A annlist

cd $outdir
rc=$?
if [[ $rc -ne 0 ]]; then
  echo "Unable to cd to $outdir"
  exit 1
fi

function mkann {
  for voice in ${!vlist[@]}; do
    lang=${vlist[$voice]}
    test -d $voice && rm -rf $voice
    mkdir $voice
    for fn in ${!annlist[@]}; do
      text=${annlist[$fn]}
      ffn=$voice/$fn
      LANG=${lang} say -v $voice "$text"
      LANG=${lang} say -v $voice -o $ffn --file-format=flac "$text"
    done
    slang=$(echo ${lang} | sed -e 's,\(..\).*,\1,')
    zfn=$slang-$voice.zip
    test -f $zfn && rm -f $zfn
    zip -q -r $zfn $voice
    rm -rf $voice   # no longer needed
  done
}

vlist=()
vlist['Samantha']='en_US.UTF8'
vlist['Daniel']='en_GB.UTF8'

# the misspellings are deliberate in order to make the voice correct.
annlist=()
# the usual
annlist['ann-argentine-tango']='Next Dance, Argenteen Tango'
annlist['ann-cha-cha']='Next Dance, Cha-Cha'
annlist['ann-foxtrot']='Next Dance, Foxtrot'
annlist['ann-jive']='Next Dance, Jive'
annlist['ann-quickstep']='Next Dance, Quickstep'
annlist['ann-rumba']='Next Dance, Rumba'
annlist['ann-intl-rumba']='Next Dance, International Rumba'
annlist['ann-salsa']='Next Dance, Salsa'
annlist['ann-samba']='Next Dance, Saamba'
annlist['ann-slow-foxtrot']='Next Dance, Slow Foxtrot'
annlist['ann-slow-waltz']='Next Dance, Slow Waltz'
annlist['ann-tango']='Next Dance, Tango'
annlist['ann-viennese-waltz']='Next Dance, Veeinnese Waltz'
annlist['ann-waltz']='Next Dance, Waltz'
annlist['ann-west-coast-swing']='Next Dance, West Coast Swing'
# uncommon
annlist['ann-paso-doble']='Next Dance, Paaso Doblay'
# american
annlist['ann-bolero']='Next Dance, Bollero'
annlist['ann-chinese-jitterbug']='Next Dance, Chinese Jitterbug'
annlist['ann-chinese-tango']='Next Dance, Chinese Tango'
annlist['ann-east-coast-swing']='Next Dance, East Coast Swing'
annlist['ann-night-club-two-step']='Next Dance, Night Club Two Step'
annlist['ann-single-swing']='Next Dance, Single Swing'
annlist['ann-bachata']='Next Dance, Bachata'
annlist['ann-hustle']='Next Dance, Hhustle'
annlist['ann-lindy-hop']='Next Dance, Lindy Hop'
annlist['ann-mambo']='Next Dance, Maambo'
annlist['ann-merengue']='Next Dance, Murreingay'
annlist['ann-polka']='Next Dance, Polka'
annlist['ann-country-western-two-step']='Next Dance, Country Western Two Step'
annlist['ann-shag']='Next Dance, Shag'
annlist['ann-bossa-nova']='Next Dance, Bossa Nova'
annlist['ann-balboa']='Next Dance, Balboa'

mkann

vlist=()
vlist['Ellen']='nl_BE.UTF8'
vlist['Xander']='nl_NL.UTF8'

# the usual
annlist=()
annlist['ann-argentijnse-tango']='Volgende dans, Argentijnse Tango'
annlist['ann-cha-cha']='Volgende dans, Cha-Cha'
annlist['ann-foxtrot']='Volgende dans, Foxtrot'
annlist['ann-jive']='Volgende dans, Jive'
annlist['ann-quickstep']='Volgende dans, Quickstep'
annlist['ann-rumba']='Volgende dans, Rumba'
annlist['ann-salsa']='Volgende dans, Salsa'
annlist['ann-samba']='Volgende dans, Saamba'
annlist['ann-slowfox']='Volgende dans, Slowfox'
annlist['ann-langzame-wals']='Volgende dans, Langzame Wals'
annlist['ann-tango']='Volgende dans, Tango'
annlist['ann-viennese-wals']='Volgende dans, Weense wals'
annlist['ann-west-coast-swing']='Volgende dans, West Coast Swing'
# uncommon
annlist['ann-paso-doble']='Volgende dans, Paaso Doble'

mkann
