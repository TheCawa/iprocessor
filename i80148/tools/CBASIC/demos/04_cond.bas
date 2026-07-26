dim x as integer
let x = 7
if x > 5 then
    print "big"
else
    print "small"
endif

select x
    case 1 to 3:
        print "low"
    case 7:
        print "seven"
    case else:
        print "other"
end select
stop
