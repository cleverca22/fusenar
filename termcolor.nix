{ stdenv, cmake, fetchFromGitHub }:

stdenv.mkDerivation {
  name = "termcolor";
  src = fetchFromGitHub {
    owner = "ikalnitsky";
    repo = "termcolor";
    rev = "bfa523101efbdd48ce26e05244bd6f81f7cf1fae";
    sha256 = "1x41q5klp90ijngaas6rmgimp3859qlv89fr93w00yzvlhvwm013";
  };
  buildInputs = [ cmake ];
}
