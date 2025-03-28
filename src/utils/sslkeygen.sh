#/bin/sh
#
# taken from mongoose/test/certs
#

while test ! \( -d src -a -d web -a -d wiki \); do
  cd ..
done
cd src
cwd=$(pwd)

clean=F
install=F
inspect=F
while test $# -gt 0; do
  case $1 in
    --clean)
      clean=T
      shift
      ;;
    --install)
      install=T
      shift
      ;;
    --inspect)
      inspect=T
      shift
      ;;
  esac
done

if [[ $inspect == T || $install == T ]]; then
  # Create cnf for adding SAN DNS:localhost
  # See https://security.stackexchange.com/questions/190905/subject-alternative-name-in-certificate-signing-request-apparently-does-not-surv
  cat > cnf <<_HERE_
[SAN]
subjectAltName=DNS:localhost
_HERE_

  #DAYS=3650
  DAYS=18250

  # Generate CA
  # Important: CN names must be different for CA and client/server certs
  openssl ecparam -noout -name prime256v1 -genkey -out ca.key
  openssl req -x509 -new -key ca.key -days ${DAYS} -subj /CN=BDJ4 -out ca.crt

  # Generate server cert
  openssl ecparam -noout -name prime256v1 -genkey -out server.key
  openssl req -new -sha256 -key server.key \
    -subj /CN=bdj4server -out server.csr
  openssl x509 -req -sha256 -in server.csr -extensions SAN -extfile cnf \
    -CAkey ca.key -CA ca.crt -CAcreateserial -days ${DAYS} -out server.crt

  # Generate client cert
  #openssl ecparam -noout -name prime256v1 -genkey -out client.key
  #openssl req -new -sha256 -key client.key -days ${DAYS} -subj /CN=client -out client.csr
  #openssl x509 -req -sha256 -in client.csr -extensions SAN -extfile cnf \
  #  -CAkey ca.key -CA ca.crt -CAcreateserial -days ${DAYS} -out client.crt

  # Verify
  openssl verify -verbose -CAfile ca.crt server.crt
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "## verification failed"
    exit 1
  fi
  #openssl verify -verbose -CAfile ca.crt client.crt
fi

if [[ $inspect == T ]]; then
  # Inspect
  openssl x509 -text -noout -ext subjectAltName -in server.crt
fi

if [[ $install == T ]]; then
  cp -f ca.crt server.crt server.key ../http
fi

if [[ $clean == T ]]; then
  # cleanup
  rm -f cnf ca.crt ca.key ca.srl server.csr server.key server.crt
fi

exit 0
