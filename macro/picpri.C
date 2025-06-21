{
  c1->SaveAs("macro/pic/tmp.png");
  system("lpr -Prcc2570A macro/pic/tmp.png -o scaling=50");
}
