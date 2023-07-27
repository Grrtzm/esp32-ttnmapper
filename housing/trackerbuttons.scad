// Afstand tussen center van buitenste knopjes: 18.6mm
// oftewel afstand tussen center van knopjes is 9.3mm
// 27mm = breedte in doosje
// 24mm is in lengterichting
// knopjes zijn 5,6mm rond

difference(){
    union(){ // geheel
        difference(){ // bodemplaat
            translate([-3,0,0])
                difference(){ // bodemplaat met lipjes
                    difference(){ // 1
                        union(){ // 1
                        translate([0,-12.5,0])
                            linear_extrude(height = 0.6)
                                square([23, 24.7]);
                        translate([18,-20,0])
                            linear_extrude(height = 0.6)
                                square([5, 40]);
                        } // einde union 1
                        translate([-1,-4.7,0])
                            rotate([0,90,0])
                                linear_extrude(height = 19)
                                    square([2.5,2.8], center = true);
                    } // einde difference() 1
                    translate([-1,4.7,0])
                        rotate([0,90,0])
                            linear_extrude(height = 19)
                                square([2.5,2.8], center = true);
                } // einde difference lipjes
        } // einde difference bodemplaat
        
        union(){ // bolletjes op lipjes
            union(){
                linear_extrude(height=2)
                    circle(d=5.6,$fn=36);
                difference(){    
                    translate([0,0,2]) // hoogte van het knopje
                        sphere(d=5.6,$fn=36);
                        translate([0,0,-3])
                            linear_extrude(height=4)
                                square([6,6], center = true);
                }
            }

            union(){
                linear_extrude(height=2)
                    circle(d=5.6,$fn=36);
                difference(){    
                    translate([0,-9.3,1]) // hoogte van het knopje
                        sphere(d=5.6,$fn=36);
                        translate([0,-9.3,-4])
                            linear_extrude(height=4)
                                square([6,6], center = true);
                }
            }
                        
            union(){
                translate([0,9.3,0])
                    difference(){    
                        sphere(d=5.6,$fn=36);
                        translate([0,0,-3])
                            linear_extrude(height=3)
                                square([6,6], center = true);
                    }
            }
        } // einde union bolletjes op lipjes
    } // einde union geheel
       
    translate([0,0,-0.1])
    union(){
        linear_extrude(height=0.6)
            circle(d=2.5,$fn=36);

        translate([0,9.3,0])
            linear_extrude(height=0.6)
                circle(d=2.5,$fn=36);
     
        translate([0,-9.3,0])
            linear_extrude(height=0.6)
                circle(d=2.5,$fn=36);
    }
}