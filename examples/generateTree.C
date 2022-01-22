{
  TFile f("TestTree.root", "RECREATE"); 
  TTree * t = new TTree("points", "Example tree with points");
  std::vector<float> x;
  std::vector<float> y;
  std::vector<float> z;
  int category = 0;
  t->Branch("x", &x);
  t->Branch("y", &y);
  t->Branch("z", &z);

  t->Branch("category", &category);

  for ( int i = 0; i < 100000; ++i ) {
      x.clear();
      y.clear();
      z.clear();

      for ( int p = 0; p < gRandom->Integer(100)+5; ++p) {
        x.push_back(gRandom->Uniform(1, 2));
        y.push_back(gRandom->Gaus( 0, std::pow(x.back(), 2)));
        z.push_back(gRandom->Gaus(std::pow(x.back(), 2)*y.back(), x.back()));
      }
      category = gRandom->Gaus( *std::max_element(z.begin(), z.end())/2.0, 1);
      if ( category < 0 ) category = 0;
      if (category > 4 ) category = 4;

    t->Fill();
  }
  f.Write();
  f.Close();
  std::cout << "... generated " << std::endl;
  exit(0);
}