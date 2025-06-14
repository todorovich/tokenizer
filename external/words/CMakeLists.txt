set(GOOGLE_WORDLIST_URL "https://raw.githubusercontent.com/first20hours/google-10000-english/master/google-10000-english.txt")
set(GOOGLE_WORDLIST_PATH "${CMAKE_BINARY_DIR}/data/google-10000-english.txt")

file(DOWNLOAD
    ${GOOGLE_WORDLIST_URL}
    ${GOOGLE_WORDLIST_PATH}
    EXPECTED_HASH SHA256=9c965d384526facc59260e94f8ccff1582633fa385004abe1455ed457062acbc
    SHOW_PROGRESS
)
