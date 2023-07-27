$fn = 36;

module zuignaphouder(){
    difference(){
        difference(){
            difference(){
                translate([0,0,5])    
                    cylinder(h = 6, d1 = 30, d2 = 20, center = true);
                translate([0,0,2])    
                    cylinder(h = 6, d1 = 25, d2 = 15, center = true);    
            }
            cylinder(h = 20, d = 7, center = true);
        }

        union(){
            translate([0,-10,6.5]) 
                cube([7,20,3], center = true);

            translate([0,-10,6.5]) 
                cube([14,8,3], center = true);

            translate([0,-10,3.5]) 
                cube([14,15,3], center = true);
        }
    }
}

//linear_extrude(height = 2)
//    square([40,40], center = true);

difference(){
    difference(){
        difference(){
            cube([109,37,44], center = true);
            translate([0,0,2])
                cube([105,33,42], center = true);
        }
        translate([0,-3,10])
            cube([86,35,42], center = true);
    }
    translate([0,0,-10])
        cube([86,28,42], center = true);
}    

translate([39,33/2,6])
    rotate([90,0,180])
        zuignaphouder();

translate([-39,33/2,6])
    rotate([90,0,180])
        zuignaphouder();

translate([0,(37/2)+4.5,-21])
    cube([109,11,2], center = true);